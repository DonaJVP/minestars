/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, Donatto J. Viveros. P. <donatto555@gmail.com>
 * Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

#include "NetworkEngine.h"
#include "../network/serveropcodes.h"
#include "../network/mt_connection.h"
#include "../itemdef.h"
#include "../craftdef.h"
#include "../emerge.h"
#include "../nodedef.h"
#include "../ServerNetworkEngine.h"
#include "../debug.h"
#include "../log.h"
#include "../remoteplayer.h"
#include "../server.h"
#include "../ban.h"

void *NetworkCompressor::run()
{
    BEGIN_DEBUG_EXCEPTION_HANDLER
    while (!stopRequested()) {
        mutex.lock();
        std::deque<NetworkPacket> queue = std::move(queue_);
        mutex.unlock();
        if (queue.size() > 0) {
            NetworkPacket pkt; //Special numer
            verbosestream << "Compressing " << queue.size() << " packets to send into main server" << std::endl;
            std::stringstream ss;
            //u8 raw_data[];
            u16 packets = 0;
            std::vector<u8> data;
            u32 sizeF = 5;

            data.resize(5);
            writeU16(&data[0], 0x9a);
            writeU8(&data[2], 0); //Marks if the packet was divided or not
            writeU16(&data[3], (u16)queue.size());

            //[0]X [1]X [2]Y [3]Y

            //We will compress the packets on the critical zone, so we can see the raw data and concatenate all to form a single packet
            while (!queue.empty()) {
                packets++;
                NetworkPacket pkt = queue.front();
                queue.pop_front();
                u8 *raw_data = pkt.getU8Ptr(0); //Get with his command so we can see how big is it
                u32 size = pkt.getSize();
                ss << "\nPacket '";
                ss << packets;
                ss << "' size: ";
                ss << size+4;
                ss << ", CMD: ";
                ss << pkt.getCommand();
                ss << ";";
                data.resize(sizeF+pkt.getSize()+4+2); //Former size + raw packet size + command size + sizeof(uint16_t)::[SIZE of PACKET]

                writeU32(&data[sizeF], size+2); //SIZE: raw packet size + commmand size

                writeU16(&data[sizeF+4], (u16)pkt.getCommand()); //CMD: command
                memcpy(&data[sizeF+6], &raw_data[0], size);  //RAW DATA

                sizeF += size + 6;
            }
            verbosestream << FUNCTION_NAME << " -> Summary: " << ss.str() << std::endl;

            //Raw write all 65535

            //u8 *p = (u8*)&data[0];
            pkt.putRawPacket((u8*)data.data(), data.size(), (session_t)1);

            //
            std::vector<NetworkPacket> to_send;
            if (pkt.getSize() > 5000) {
                //If the packet are too big to be sent (limit=16777216bytes) we will divide the packets, with a size of 5000bytes-max
                //It really will be 5004 bytes as the size byte we need and the order for mini packets
                warningstream << "Packet reaches limit of size: " << pkt.getSize() << ", limiting" << std::endl;
                if (pkt.getSize() > 700000) {
                    errorstream << "Packet too big" << std::endl;
                }
                //u32 remaining_size = pkt.getSize();
                u32 total_size = pkt.getSize();
                u16 sb_ = (total_size/5000)+1; //+2
                //u8 fuckall = 2;
                u32 read_size = 0;
                u16 order_num = 0;
                std::vector<u16> counts;
                for (u8 i = 0; i < sb_; i++) {
                    order_num++;
                    //Get the 5000bytes of the packet to form a single packet and push into to_send;
                    u8 *raw_data_ = nullptr;
                    try {
                        verbosestream << FUNCTION_NAME << ": Program: read_size=" << read_size << std::endl;
                        raw_data_ = pkt.getU8Ptr(read_size);
                    } catch (std::exception &e) {
                        //This can occur every time.
                        //We need the last bytes to be an complete packet
                        verbosestream << FUNCTION_NAME << ": catch(exception)" << std::endl;
                        //fuckall = 0;
                        break;
                    }
                    std::vector<u8> data_RW;
                    data_RW.resize(2+2+2+1+5000);
                    //First number(u16): Order, Second number(u16): Size, Raw data: Raw data.

                    writeU16(&data_RW[0], 0x9a);          // 2 X
                    writeU8(&data_RW[2], 1);              // 1 _
                    writeU16(&data_RW[3], sb_);           // 2 _
                    writeU16(&data_RW[5], order_num);     // 2 _
                    memcpy(&data_RW[7], &raw_data_[0], 5000); //-
                    //writeU16(&data_RW[6], data_RW.size()-(2+1+1+2)); //2
                    read_size += data_RW.size()-7; //its the total size
                    NetworkPacket tmp;
                    tmp.putRawPacket((u8*)data_RW.data(), data_RW.size(), (session_t)1);
                    verbosestream << FUNCTION_NAME << ": ToSend: Packed: " << order_num << ", <size=" << data_RW.size() << ", t_size=" << tmp.getSize() << ">" << std::endl;
                    to_send.push_back(tmp);
                }
                for (NetworkPacket &pkt_i : to_send) {
                    m_server->Send(&pkt_i);
                }
            } else {
                m_server->Send(&pkt);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    END_DEBUG_EXCEPTION_HANDLER
    return nullptr;
}

void NetworkCompressor::queuePacket(NetworkPacket pkt) {
    mutex.lock();
    queue_.push_back(pkt);
    mutex.unlock();
}

class _NetworkThreadRecv: public Thread {
public:
    _NetworkThreadRecv(Server *OBJ): m_server(OBJ), Thread("_NetworkThreadRecv") {}
    void *run() {
        actionstream << "Starting _NetworkThreadRecv" << std::endl;
        while (!stopRequested()) {
            m_server->Receive();
        }
    }
private:
    Server *m_server = nullptr;
};

const uint32_t MAX_PACKET_SIZE = 64*1024*64;

void *NetworkThread::run() {
    BEGIN_DEBUG_EXCEPTION_HANDLER
    if (!m_server->SocketConn) {
        while (!stopRequested()) {
            try {
                m_server->Receive(); //When receiving packets of movements, must be sent directly to other players without any control
            } catch (con::PeerNotFoundException &e) {
                infostream<<"Server: PeerNotFoundException"<<std::endl;
            } catch (ClientNotFoundException &e) {
            } catch (con::ConnectionBindFailed &e) {
                m_server->setAsyncFatalError(e.what());
            }
        }
    } else {
        // Special thread to receive data
        _NetworkThreadRecv *Trecv = new _NetworkThreadRecv(m_server);
        //Connect socket N continue
        //Make connection
        //errorstream <<m_server->SocketID<<std::endl;
        actionstream << "Going to connect to socket!" << std::endl;
        if (listen(m_server->SocketID, 1) == -1) {
            errorstream << "Unable to listen to main socket for server: " << std::endl;
            return nullptr;
        }
        actionstream << "Listening complete!" << std::endl;
        int client = accept(m_server->SocketID, nullptr, nullptr);
        if (client == -1) {
            errorstream << "Unable to listen to client in socket for server" << std::endl;
            return nullptr;
        }
        actionstream << "Connected!" << std::endl;
        sockaddr_un client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        NetworkPacket pkt;
        Trecv->start();
        while (!stopRequested()) {
            //warningstream << "Waiting" << std::endl;
            std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
            ssize_t bytes_received = recv(client, &buffer[0], MAX_PACKET_SIZE, 0);
            if (bytes_received > 0) {
                warningstream << FUNCTION_NAME << ": Received: " << bytes_received << std::endl;
                pkt.putRawPacket(buffer.data(), bytes_received, 0);
                _mutex.lock();
                packets.push_back(pkt);
                _mutex.unlock();
            }
            // Packets to send buffer
            lock();
            while (!packets_ts.empty()) {
                NetworkPacket pkt = packets_ts.front();
                std::vector<uint8_t> raw_data;
                raw_data.resize(2+pkt.getSize());
                writeU16(&raw_data[0], pkt.getCommand());
                memcpy(&raw_data[2], pkt.getU8Ptr(0), pkt.getSize());
                if (send(client, &raw_data[0], raw_data.size(), 0) == -1) {
                    errorstream << FUNCTION_NAME << ": Sending packet failed! <" << errno << "; " << strerror(errno) << ">\nPacketSize: "<<raw_data.size()<<"; Socket="<<client << std::endl;
                } else {
                    packets_ts.pop_front();
                    actionstream << "Sent pkt <" << pkt.getSize() << ">" << std::endl;
                }
            }
            unlock();
        }
        Trecv->wait();
        Trecv->stop();
        delete Trecv;
    }
    END_DEBUG_EXCEPTION_HANDLER
    return nullptr;
}
void NetworkThread::queue(NetworkPacket pkt) {lock(); packets_ts.push_back(pkt); unlock();}

#include <errno.h>

void Server::SendToMainServer(NetworkPacket *pkt) {
    if (!SocketConn) {
        NetCR->queuePacket(*pkt);
    } else {
        //Send a raw big packet
        /*std::vector<uint8_t> raw_data;
        raw_data.resize(2+pkt->getSize());
        writeU16(&raw_data[0], pkt->getCommand());
        memcpy(&raw_data[2], pkt->getU8Ptr(0), pkt->getSize()+2);
        if (send(m_netthr->m_parent_skt.load(), &raw_data[0], raw_data.size(), 0) == -1) {
            errorstream << FUNCTION_NAME << ": Sending packet failed! <" << errno << "; " << strerror(errno) << ">\nPacketSize: "<<raw_data.size()<<"; Socket="<<m_netthr->m_parent_skt.load() << std::endl;
        }*/
        m_netthr->queue(*pkt);

    }
}

NetworkPacket NetworkThread::getPacket() {
    _mutex.lock();
    NetworkPacket pkt;
    if (!packets.empty()) {
        pkt = (packets.size() > 0) ? packets.front() : NetworkPacket();
        packets.pop_front();
    }
    _mutex.unlock();
    return pkt;
}

//BEGIN SERVER

void Server::QueueProcessData(NetworkPacket pkt) {
    QueuedPackets.push(pkt);
}

void Server::handleCommand(NetworkPacket *pkt)
{
    //This should handle player interactions with the server...
    const ToServerCommandHandler &opHandle = toServerCommandTable[pkt->getCommand()];
    (this->*opHandle.handler)(pkt);
}

void Server::ProcessData(NetworkPacket *pkt)
{
    // Environment is locked first.
    MutexAutoLock envlock(m_env_mutex);
    u32 peer_id = pkt->getPeerId();

    if (ServersNetworkObject->AreSlave)
        goto sectorrun;

    if (EnabledPlayers.size() > peer_id && EnabledPlayers[peer_id]) { //If not verified then check those first packets
        goto sectorrun;
    }

    sectorcheck:
    {
        try {
            Address address = getPeerAddress(peer_id);
            if (address.serializeString() == "255.255.255.255") {
                warningstream << "Skipped peerId: " <<peer_id << " due to early check, queuing for later use." << std::endl;
                QueueProcessData(*pkt);
                return; // Skip
            }
            std::string addr_s = address.serializeString();

            // FIXME: Isn't it a bit excessive to check this for every packet?
            if (m_banmanager->isIpBanned(addr_s)) {
                std::string ban_name = m_banmanager->getBanName(addr_s);
                infostream << "Server: A banned client tried to connect from " << addr_s << "; banned name was " << ban_name << std::endl;
                DenyAccess(peer_id, SERVER_ACCESSDENIED_CUSTOM_STRING, "Your IP is banned. Banned name was " + ban_name);
                return;
            } else {
                verbosestream << FUNCTION_NAME << ": Verified player: " << peer_id << std::endl;
                if (EnabledPlayers.size() < (peer_id+1)) {
                    EnabledPlayers.resize(peer_id+1);
                }
                EnabledPlayers[peer_id] = true;
            }
        } catch (con::PeerNotFoundException &e) {
            /*
            * no peer for this packet found
            * most common reason is peer timeout, e.g. peer didn't
            * respond for some time, your server was overloaded or
            * things like that.
            */
            infostream << "Server::ProcessData(): Canceling: peer "
            << peer_id << " not found" << std::endl;
            return;
        }
    }

    sectorrun:
    {
        try {
            ToServerCommand command = (ToServerCommand) pkt->getCommand();

            // Command must be handled into ToServerCommandHandler
            if (command >= TOSERVER_NUM_MSG_TYPES) {
                infostream << "Server: Ignoring unknown command "
                << command << std::endl;
                return;
            }

            if (toServerCommandTable[command].state == TOSERVER_STATE_NOT_CONNECTED) {
                handleCommand(pkt);
                return;
            }

            RemoteClient *client = getClient(peer_id, CS_InitDone);
            
            if (!client) {
                return;
            }
            
            u8 peer_ser_ver = client->serialization_version;

            if(peer_ser_ver == SER_FMT_VER_INVALID) {
                errorstream << "Server::ProcessData(): Cancelling: Peer"
                " serialization format invalid or not initialized."
                " Skipping incoming command=" << command << std::endl;
                return;
            }

            /* Handle commands related to client startup */
            if (toServerCommandTable[command].state == TOSERVER_STATE_STARTUP) {
                handleCommand(pkt);
                return;
            }

            if (m_clients.getClientState(peer_id) < CS_Active) {
                if (command == TOSERVER_PLAYERPOS) return;

                errorstream << "Got packet command: " << command << " for peer id "
                << peer_id << " but client isn't active yet. Dropping packet "
                << std::endl;
                return;
            }

            if (!ServersNetworkObject->AreSlave) {
                RemoteClient *client = getClient(peer_id);
                if (client)
                    pkt->setProtocolVersion(client->net_proto_version);
            } else {
                ClientDataHelper *client = nullptr;
            }
            handleCommand(pkt);
        } catch (SendFailedException &e) {
            errorstream << "Server::ProcessData(): SendFailedException: "
            << "what=" << e.what()
            << std::endl;
        } catch (PacketError &e) {
            actionstream << "Server::ProcessData(): PacketError: "
            << "what=" << e.what() << ", command=" << pkt->getCommand() // TODO: REMOVE COMMAND=
            << std::endl;
        }
    }
}

void Server::Receive()
{
    NetworkPacket pkt;
    if (!SocketConn) {
        session_t peer_id;
        bool first = true;
        for (;;) {
            pkt.clear();
            peer_id = 0;
            try {
                if (first) {
                    m_con->Receive(&pkt);
                    first = false;
                } else {
                    if (!m_con->TryReceive(&pkt))
                        return;
                }

                peer_id = pkt.getPeerId();

                m_packet_recv_counter->increment();

                if (ServersNetworkObject->IsThisAProxy) {
                    if (PeerIdPlayers.Has(peer_id)) {
                        if (!PeerIdPlayers.Get(peer_id)->PlayingOnServ)
                            goto __c_recv;
                        u16 ServID = PeerIdPlayers.Get(peer_id)->ServerID;
                        try {
                            ServersNetworkObject->RedirectPacket(&pkt, ServID);
                        } catch (const FailureOnIndexingARegisteredServer &e) {
                            errorstream << "RedirectPacket(): " << e.what() << std::endl;
                        }
                        //infostream << "Redirecting packet to server "<<ServID << std::endl;
                        return;
                    }
                }

                if (pkt.getCommand() >= 0x57) {
                    actionstream << "Got proxy command: " << pkt.getCommand() << std::endl;
                    HandleProxyCommand(&pkt);
                    return;
                }

                __c_recv:

                //When queued packets it will be executed in AsyncRunStep
                if (!ServersNetworkObject->AreSlave)
                    QueueProcessData(pkt);
                else
                    ProcessData(&pkt);
                m_packet_recv_processed_counter->increment();
            } catch (const con::InvalidIncomingDataException &e) {
                infostream << "Server::Receive(): InvalidIncomingDataException: what()="
                << e.what() << std::endl;
            } catch (const SerializationError &e) {
                infostream << "Server::Receive(): SerializationError: what()="
                << e.what() << std::endl;
            } catch (const ClientStateError &e) {
                errorstream << "ProcessData: peer=" << peer_id << " what()="
                << e.what() << std::endl;
                DenyAccess(peer_id, SERVER_ACCESSDENIED_UNEXPECTED_DATA);
            } catch (const con::NoIncomingDataException &e) {
                return;
            }
        }
    } else {
        try {
            NetworkPacket pkt = m_netthr->getPacket();
            if (pkt.getCommand() != 0x0) {
                if (pkt.getCommand() >= 0x57) {
                    actionstream << "Got proxy command: " << pkt.getCommand() << std::endl;
                    HandleProxyCommand(&pkt);
                    return;
                }
                m_packet_recv_counter->increment();
                ProcessData(&pkt);
                m_packet_recv_processed_counter->increment();
            }
        } catch (const SerializationError &e) {
            warningstream << "Server::Receive(): SerializationError: what()=" << e.what() << std::endl;
        }
    }
}

void Server::HandleProxyCommand(NetworkPacket *pkt) {
    //This will handle this commands:
    /*
     * TOSERVER_GOT_DISCONNECT
     * TOSERVER_GOT_CONNECT
     * 0x95: Handle reserve player sao ID
     * 0x96: Handle 'SendMeItemDefinitions'
     * 0x97: Handle 'SendMeNodeDefinitions'
     * 0x98: Handle 'JumpDefinitionsAtNodeDef'
     * 0x99: Handle 'JumpDefinitionsAtItemDef'
     */
    u16 CMD = pkt->getCommand();
    switch (CMD) {
        case 0x101: {
            //Main proxy requires initialization data for player, but in AOM format
            uint16_t id, player;
            *pkt >> id >> player;
            ServerActiveObject* obj = m_env->getActiveObject(id);
            if (obj != nullptr) {
                NetworkPacket pkt(0x77, 0, player, 0, ThisServID);
                std::string data = obj->GetInitData(32);
                std::string buff_;
                //char bf[2]; //NOTE: Not needed because of Send()
                //writeU8((u8*)bf, (u8)ServersNetworkObject->QueryThisServerID());
                //buff_.append(bf, 1);
                //writeU16((u8*)bf, player->player_id);
                //buff_.append(bf, 2);
                buff_.append(data);
                pkt.putRawString(buff_.c_str(), buff_.size());
                Send(&pkt);
            }
            return;
        }
        case 0x110: {
            //Server requires media data raw
            // Read data
            u16 count = 0;
            *pkt >> count;

            std::ostringstream toserv(std::ios_base::binary);

            for (u16 i = 0; i < count; i++) {
                std::string name;
                *pkt >> name;
                std::string tpath = m_media[name].path;
                std::ifstream fis(tpath.c_str(), std::ios_base::binary);
                if(!fis.good()){
                    errorstream<<"Server[External]::0x110(): Could not open \""<<tpath<<"\" for reading"<<std::endl;
                    continue;
                }
                std::ostringstream tmp_os(std::ios_base::binary);
                bool bad = false;
                for(;;) {
                    char buf[1024];
                    fis.read(buf, 1024);
                    std::streamsize len = fis.gcount();
                    tmp_os.write(buf, len);
                    if(fis.eof())
                        break;
                    if(!fis.good()) {
                        bad = true;
                        break;
                    }
                }
                if (bad) {
                    errorstream<<"Server[External]::0x110(): Failed to read \""<<name<<"\""<<std::endl;
                    continue;
                }

                toserv << serializeString16(name);
                toserv << serializeString64(tmp_os.str());
            }

            NetworkPacket to_send(0x75, 0);
            to_send << (u8)ServersNetworkObject->QueryThisServerID();
            to_send << count;
            to_send.putLongString(toserv.str());
            Send(&to_send);
            return;
        }
        case 0x116: {
            //Server requires media data sums
            // Make packet
            NetworkPacket pkt2(0x74, 0);
            pkt2 << (u8)ServersNetworkObject->QueryThisServerID();
            u16 media_sent = 0;
            std::string lang_suffix;
            for (const auto &i : m_media) {
                media_sent++;
            }
            pkt2 << (u16)m_media.size();
            for (const auto &i : m_media) {
                pkt2 << i.first;
                pkt2 << i.second.sha1_digest;
            }
            Send(&pkt2);
            verbosestream << "Server: Announcing files to proxy: count=" << media_sent << " size=" << pkt2.getSize() << std::endl;
            return;
        }
        case 0x99: { //Proxy requires the data of definitions
            //sendDefinitions();
            return;
        }
        case 0x98: { //Perform a update to ItemDefinitions and NodeDefinitions
            MutexAutoLock envlock(m_env_mutex);

            DEFINITIONS_EXECUTION.lock();

            //Copy all nodes which we will edit
            u16 count = 0;
            *pkt >> count;
            for (u16 i = 0; i < count; i++) {
                u16 original_id, new_id = 0;
                *pkt >> original_id >> new_id;
                Jumper[original_id] = new_id;
                verbosestream << FUNCTION_NAME << ": ToCopy: " << original_id << " | newID: " << new_id << std::endl;
            }

            //Here we initialize the big machine
            m_nodedef->updateAliases(m_itemdef);
            m_nodedef->setNodeRegistrationStatus(true);
            // Perform pending node name resolutions
            m_nodedef->runNodeResolveCallbacks();
            // unmap node names in cross-references
            m_nodedef->resolveCrossrefs();
            // init the recipe hashes to speed up crafting
            m_craftdef->initHashes(this);

            m_emerge->initMapgens(m_startup_server_map->getMapgenParams());

            m_definitions_done = true;

            DEFINITIONS_EXECUTION.unlock();

            return;
        }
        case 0x97: { //PlayerPOS compressed
            handleCommand_PlayerPosAdvanced(pkt);
            return;
        }
        case 0x59: {
            handleCommand_GotConnect(pkt);
            return;
        }
        case 0x58: {
            handleCommand_GotDisconnect(pkt);
            return;
        }
        default: {
            warningstream << "Unknown command base: " << CMD << std::endl;
            return;
        }
    }
}

void Server::DoSendToEveryone(NetworkPacket *pkt, NetworkPacket *legacy_pkt, u16 SID) {
    for (auto it = Servers[SID].begin(); it != Servers[SID].end(); it++) {
        if (it->second->ServerID == SID) {
            RemotePlayer *p = m_env->getPlayer(it->first);
            if (p) {
                if (p->protocol_version >= 37) {
                    Send(it->first, pkt);
                } else {
                    Send(it->first, legacy_pkt);
                }
            }
        }
    }
}

void Server::sendUpdatePlayerSaoList(u16 playerid, std::unordered_map<u16, u16> playerlistNsao) {
    NetworkPacket pkt(0x69, 0);
    pkt << playerid << playerlistNsao.size();
    for (auto it = playerlistNsao.begin(); it != playerlistNsao.end(); it++) {
        pkt << it->first << it->second; //first=PlayerID, second=PlayerSAO (depends on every player)
    }
    Send(&pkt);
}

#include "network/address.h"

Address Server::getPeerAddress(session_t peer_id)
{
    // Note that this is only set after Init was received in Server::handleCommand_Init
    RemoteClient *clt = getClient(peer_id, CS_Invalid);
    if (!clt) {
        return Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFFFF);
    }
    return clt->getAddress();
}

//BEGIN Custom_Server_Send_Func

void Server::SendAccessDenied(session_t peer_id, AccessDeniedCode reason, const std::string &custom_reason, bool reconnect)
{
    assert(reason < SERVER_ACCESSDENIED_MAX);
    NetworkPacket pkt(TOCLIENT_ACCESS_DENIED, 1, peer_id);
    pkt << (u8)reason;
    if (reason == SERVER_ACCESSDENIED_CUSTOM_STRING)
        pkt << custom_reason;
    else if (reason == SERVER_ACCESSDENIED_SHUTDOWN ||
        reason == SERVER_ACCESSDENIED_CRASH)
        pkt << custom_reason << (u8)reconnect;
    Send(&pkt);
}

void Server::Send(NetworkPacket *pkt, bool relative)
{
    if (ServersNetworkObject->IsThisAProxy)
        Send(pkt->getPeerId(), pkt, relative);
    else
        SendToMainServer(pkt);
}

void Server::Send(NetworkPacket *pkt)
{
    if (ServersNetworkObject->IsThisAProxy)
        Send(pkt->getPeerId(), pkt);
    else {
        //Packet has metadata, so we will modify it for perfect transmission
        uint32_t pos = 0;
        std::vector<uint8_t> raw_data;
        raw_data.resize(pkt->getSize()+1+2); //2Bytes: CMD; 1Byte: ServerID
        //Useful data
        writeU8((uint8_t*)&raw_data[0], pkt->getCommand());
        writeU8((uint8_t*)&raw_data[2], pkt->getServerId());
        pos += 3;
        //Check if player id is available [Byte 3]
        if (pkt->getPeerId() != 0 && pkt->getPeerId() != PEER_ID_INEXISTENT) {
            raw_data.resize(pkt->getSize()+1+2+2);
            writeU16((uint8_t*)&raw_data[1], pkt->getPeerId());
            pos += 2; //Raw bytes of player id
        }
        //Copy the data
        memcpy(&raw_data[pos], pkt->getU8Ptr(0), pkt->getSize());
        //Set data
        pkt->putRawPacket(&raw_data[0], raw_data.size(), (session_t)1);
        SendToMainServer(pkt);
    }
}

void Server::Send(session_t peer_id, NetworkPacket *pkt, bool relative) {
    m_clients.send(peer_id, pkt->didSetChannel() ? pkt->getChannel() : clientCommandFactoryTable[pkt->getCommand()].channel, pkt, relative);
}

void Server::Send(session_t peer_id, NetworkPacket *pkt) {
    //verbosestream << "Used ::Send() with builtin reliable option" << std::endl;
    m_clients.send(peer_id, clientCommandFactoryTable[pkt->getCommand()].channel, pkt, clientCommandFactoryTable[pkt->getCommand()].reliable);
}

//END Custom_Server_Send_Func

//BEGIN PeerID

void Server::peerAdded(con::Peer *peer)
{
    verbosestream<<"Server::peerAdded(): peer->id=" <<peer->id<<std::endl;

    if (!ServersNetworkObject->AreSlave)
        m_peer_change_queue.push(con::PeerChange(con::PEER_ADDED, peer->id, false));
}

void Server::deletingPeer(con::Peer *peer, bool timeout)
{
    verbosestream<<"Server::deletingPeer(): peer->id=" <<peer->id<<", timeout="<<timeout<<std::endl;

    //Should check!

    m_clients.event(peer->id, CSE_Disconnect);
    m_peer_change_queue.push(con::PeerChange(con::PEER_REMOVED, peer->id, timeout));
}

void Server::handlePeerChanges()
{
    while(!m_peer_change_queue.empty())
    {
        con::PeerChange c = m_peer_change_queue.front();
        m_peer_change_queue.pop();

        verbosestream<<"Server: Handling peer change: "
        <<"id="<<c.peer_id<<", timeout="<<c.timeout
        <<std::endl;

        switch(c.type)
        {
            case con::PEER_ADDED:
                m_clients.CreateClient(c.peer_id);
                break;

            case con::PEER_REMOVED:
                DeleteClient(c.peer_id, c.timeout?CDR_TIMEOUT:CDR_LEAVE);
                break;

            default:
                FATAL_ERROR("Invalid peer change event received!");
                break;
        }
    }
}

//END PeerID

//BEGIN misc

void Server::SendDisconnectToPlayer(u16 ID) {
    NetworkPacket pkt(0x65, 0, ID, 0, (uint8_t)ServersNetworkObject->QueryThisServerID());
    pkt << (u8) 0 << (u8) 0;
    Send(&pkt);
}

//END misc

//END SERVER
