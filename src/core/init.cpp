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


#include "../server.h"
#include <cstdint>
#include <iostream>
#include <queue>
#include <random>
#include <algorithm>
#include <stdexcept>
#include "../debug.h"
#include "../network/mt_connection.h"
#include "../network/networkpacket.h"
#include "../network/networkprotocol.h"
#include "../network/serveropcodes.h"
#include "../ban.h"
#include "../environment.h"
#include "../map.h"
#include "../threading/mutex_auto_lock.h"
#include "../constants.h"
#include "../voxel.h"
#include "../config.h"
#include "../version.h"
#include "../filesys.h"
#include "../mapblock.h"
#include "../server/serveractiveobject.h"
#include "../settings.h"
#include "../profiler.h"
#include "../log.h"
#include "../script/scripting_server.h"
#include "../nodedef.h"
#include "../itemdef.h"
#include "../craftdef.h"
#include "../emerge.h"
#include "../mapgen/mapgen.h"
#include "../mapgen/mg_biome.h"
#include "../content_mapnode.h"
#include "../content_nodemeta.h"
#include "../content/mods.h"
#include "../modchannels.h"
#include "../serverlist.h"
#include "../util/string.h"
#include "../util/serialize.h"
#include "../util/thread.h"
#include "../defaultsettings.h"
#include "../server/mods.h"
#include "../util/base64.h"
#include "../util/sha1.h"
#include "../util/hex.h"
#include "../database/database.h"
#include "../chatmessage.h"
#include "../chat_interface.h"
#include "../remoteplayer.h"
#include "../server/player_sao.h"
#include "../server/serverinventorymgr.h"
#include "../translation.h"
#include "../ServerNetworkEngine.h"
#include "../slave_helpers.h"
#include "../slave_proxy_net/objects_id_logic.h"
#include "../addons/addons.hpp"
#include "addons/cblks_def.hpp"
#include "NetworkEngine.h"

uint16_t ThisServID = 0; //Changed when the server initializes

//NOTE: I DON'T LIKE THOSE FUNCTIONS THAT SAVES CYCLE INFOS ON THE MEMORY, SO I'VE DELETED THOSE. SORRY.

//FIXME: As an option for multithreaded mods with their own threads, add Haxe scripting

class ServSectorPlayers: public Thread {
public:
	ServSectorPlayers(ServerEnvironment *s): Thread("ServPlayers"), m_env(s) {};
	void *run() {
		while (!stopRequested()) {
			m_env->stepPlayers();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
private:
	ServerEnvironment *m_env = nullptr;
};

class ServSectorEnvChild: public Thread {
public:
	ServSectorEnvChild(ServerEnvironment *s): Thread("ServEnv"), m_env(s) {};
	void *run() {
		while (!stopRequested()) {
			m_env->step(0.01f);
            m_env->stepScript(0.01f);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return nullptr;
	}
private:
	ServerEnvironment *m_env = nullptr;
};

class ServSectorCoreStep: public Thread {
public:
    friend class Server;
    ServSectorCoreStep(Server* serv): Thread("ServSave"), m_env(serv->m_env) {
        m_banmanager = serv->m_banmanager;
    };
    void *run() {
        while (!stopRequested()) {
            static const float map_timer_and_unload_dtime = 2.92;
            if (m_map_timer_and_unload_interval.step(0.1f, map_timer_and_unload_dtime)) {
                // Run Map's timers and unload unused data
                //m_env->getMap().timerUpdate(map_timer_and_unload_dtime, g_settings->getFloat("server_unload_unused_data_timeout"), U32_MAX);
            }
            m_savemap_timer += 0.1f;
            static thread_local const float save_interval = g_settings->getFloat("server_map_save_interval");
            if (m_savemap_timer >= save_interval) {
                m_savemap_timer = 0.0;
                // Save ban file
                if (m_banmanager->isModified()) {
                    m_banmanager->save();
                }
                // Save changed parts of map
                m_env->getMap().save(MOD_STATE_WRITE_NEEDED);
                // Save players
                m_env->saveLoadedPlayers();
                // Save environment metadata
                m_env->saveMeta();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
private:
    ServerEnvironment *m_env = nullptr;
    BanManager *m_banmanager = nullptr;
    float m_savemap_timer = 0.0f;
    IntervalLimiter m_map_timer_and_unload_interval;
};

// Server Environment Sector
class ServSectorEnv: public Thread {
public:
    ServSectorEnv(Server *s): Thread("S.A.[Recv]"), core(s) {
        SSEC = new ServSectorEnvChild(s->m_env);
        SSEC->start();
        SSP = new ServSectorPlayers(s->m_env);
        SSP->start();
        SSCP = new ServSectorCoreStep(s);
        SSCP->start();
    }
    ~ServSectorEnv() {
        SSEC->stop();
        SSEC->wait();
        SSP->stop();
        SSP->wait();
        SSCP->stop();
        SSCP->wait();
        delete SSCP;
        delete SSEC;
        delete SSP;
    }
    void *run() {
        uint16_t timer = 0;
        while (!stopRequested()) {
            while (!core->QueuedPackets.empty()) {
                NetworkPacket pkt = core->QueuedPackets.pop();
                core->ProcessData(&pkt);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return nullptr;
    }
private:
    Server *core = nullptr;
    ServSectorEnvChild *SSEC = nullptr;
    ServSectorPlayers *SSP = nullptr;
    ServSectorCoreStep *SSCP = nullptr;
};

class ServInternal: public Thread {
public:
    ServInternal(Server *serv): Thread("ServInternal"), m_server(serv) {}
    void *run() {
        if (m_server->ServersNetworkObject->AreSlave)
            while (!stopRequested()) { m_server->AsyncRunStepSlave(false); }
        else
            while (!stopRequested()) { m_server->AsyncRunStepMain(false); }
        return nullptr;
    }
private:
    Server *m_server = nullptr;
};

//This must be on NetworkEngnine, bu i'm lazy to it will don't :>
class SentNetworkThread: public Thread {
public:
    SentNetworkThread(Server *Serv): Thread("Network[Sent]"), m_server(Serv) {}
    void *run() {
        while (!stopRequested()) {
            m_server->PacketsDequeMTX.lock();
            if (!m_server->PacketsDeque.empty()) {
                NetworkPacket pkt = m_server->PacketsDeque.front();
                //actionstream << "Got packet to stream: " << pkt.getPeerId() << " | cmd=" << pkt.getCommand() << std::endl;
                if (!pkt.didSetReliableOpt())
                    m_server->Send(&pkt, pkt.reliableOption());
                else
                    m_server->Send(&pkt);
                m_server->PacketsDeque.pop_front();
            }
            m_server->PacketsDequeMTX.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return nullptr;
    }
private:
    std::mutex mtx;
    Server *m_server = nullptr;
};

//FIXME: Make this compatible with multimap
class MapUpdate: public Thread {
public:
    MapUpdate(Server *c, ServerEnvironment *env): core(c), m_env(env), Thread("MapManagement") {}
    void *run() {
    	while (!stopRequested()) {
            bool disable_single_change_sending = false;
            if(m_unsent_map_edit_queue.size() >= 4)
                disable_single_change_sending = true;
            std::list<v3s16> node_meta_updates;
            while (!m_unsent_map_edit_queue.empty()) {
                MapEditEvent *event = m_unsent_map_edit_queue.pop();
                if (event == nullptr) //FIXME: Should fix this
                    continue;
                std::unordered_set<u16> far_players;
                switch (event->type) {
                    case MEET_ADDNODE:
                    case MEET_SWAPNODE:
                        core->sendAddNode(event->p, event->n, &far_players, disable_single_change_sending ? 5 : 30, event->type == MEET_ADDNODE);
                        break;
                    case MEET_REMOVENODE:
                        core->sendRemoveNode(event->p, &far_players, disable_single_change_sending ? 5 : 30);
                        break;
                    case MEET_BLOCK_NODE_METADATA_CHANGED: {
                        if (!event->is_private_change) {
                            // Don't send the change yet. Collect them to eliminate dupes.
                            node_meta_updates.remove(event->p);
                            node_meta_updates.push_back(event->p);
                        }
                        if (MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(getNodeBlockPos(event->p))) {
                            block->raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_REPORT_META_CHANGE);
                        }
                        break;
                    }
                    case MEET_OTHER:
                        if (!core->ServersNetworkObject->AreSlave) {
                            core->m_clients.lock();
                            for (const v3s16 &modified_block : event->modified_blocks) {
                                core->m_clients.markBlockposAsNotSent(modified_block);
                            }
                            core->m_clients.unlock();
                        }
                        break;
                    default:
                        break;
                }
                if (!far_players.empty()) {
                    // Convert list format to that wanted by SetBlocksNotSent
                    std::map<v3s16, MapBlock*> modified_blocks2;
                    for (const v3s16 &modified_block: event->modified_blocks) {
                        modified_blocks2[modified_block] = m_env->getMap().getBlockNoCreateNoEx(modified_block);
                    }
                    // Set blocks not sent LLK
                    if (!core->ServersNetworkObject->AreSlave) {
                        for (const u16 far_player: far_players) {
                            try {
                                RemoteClient *client = core->getClient(far_player);
                                client->SetBlocksNotSent(modified_blocks2);
                            } catch (std::exception &e) {
                            }
                        }
                    } else {
                        for (const u16 far_player: far_players) {
                            if (!core->ClientDataTable.Has(far_player))
                                continue;
                            ClientDataHelper *client = core->ClientDataTable.Get(far_player);
                            if (client)
                                client->SetBlocksNotSent(modified_blocks2);
                        }
                    }
                }
                delete event;
            }
            if (node_meta_updates.size() > 0)
                core->sendMetadataChanged(node_meta_updates);
        }
        return nullptr;
    }
    void add(MapEditEvent *e) {
        m_unsent_map_edit_queue.push(e);
    }
private:
    //MultithreadQueue<MapEditEvent*> *m_unsent_map_edit_queue = nullptr;
    MultithreadQueue<MapEditEvent*> m_unsent_map_edit_queue;
    Server *core = nullptr;
    ServerEnvironment *m_env = nullptr;
};

struct _aom_hlp { std::string a; std::string b; u16 id; u8 cmd; };
class ThreadingAOM: public Thread {
public:
    ThreadingAOM(Server *serv, ServerEnvironment *env, bool *_s, ClientInterface *ci): Thread("T.AOM"), m_server(serv), m_env(env) {
        s = _s;
    }
    void *run() {
        while (!stopRequested()) {
            //verbosestream << "run" << std::endl;
            acquiredata();
            if (!m_server->ServersNetworkObject->AreSlave)
                mainf();
            else
                slavef();
            //Pause a little
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return nullptr;
    }
    void slavef() {
        NetworkPacket pkt(0x73, 0);
        uint16_t messages_count = 0;
        std::vector<_aom_hlp> messages_rel;
        std::vector<_aom_hlp> messages;
        std::string msg;
        //Compile and serialize every time the objs messages
        for (const auto &buffered_message : buffered_messages) {
            uint16_t id = buffered_message.first;
            ServerActiveObject *sao = m_env->getActiveObject(id);

            if (!sao)
                continue;
            /*
             * [id -  u16]
             * [reliable - u8]
             * [lengh of 'message old_version' - u16]
             * [lengh of 'message new_version' - u16]
             * [message old_version]
             * [message new_version]
             *
             * NOTE: IDs are replaced on proxy by an ID helper
             */
            std::vector<ActiveObjectMessage> *list = buffered_message.second;
            for (const ActiveObjectMessage &aom : *list) {
                //Filter some things
                if ((aom.datastring[0] == AO_CMD_UPDATE_POSITION) && sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
                    //This will be handled by multiserver_env
                    continue; //If Teleports, then the slave server will send a special packet for multiserver_env
                }
                if (aom.reliable) {
                    _aom_hlp strs = {serializeString16(aom.datastring), serializeString16(aom.legacystring), aom.id, (uint8_t)aom.datastring[0]};
                    messages_rel.push_back(strs);
                } else {
                    _aom_hlp strs = {serializeString16(aom.datastring), serializeString16(aom.legacystring), aom.id, (uint8_t)aom.datastring[0]};
                    messages.push_back(strs);
                }
            }
            messages_count++;
        }
        if (messages_count > 0) {
            //Prepare to send
            pkt << messages_count;
            //Send reliable, then unreliable
            for (const _aom_hlp &MSG : messages_rel) {
                pkt << MSG.id;
                pkt << (u8)true;
                //pkt << (u32)MSG.b.size();
                //pkt << (u32)MSG.a.size();
                pkt << (u16)MSG.cmd; //to don't seek into the string directly
                //pkt.putRawString(MSG.b.c_str(), MSG.b.size());
                //pkt.putRawString(MSG.a.c_str(), MSG.a.size());

                pkt.putLongString(MSG.b);
                pkt.putLongString(MSG.a);
            }
            for (const _aom_hlp &MSG : messages) {
                pkt << MSG.id;
                pkt << (u8)false;
                //pkt << (u32)MSG.b.size();
                //pkt << (u32)MSG.a.size();
                pkt << (u16)MSG.cmd; //to don't seek into the string directly
                //pkt.putRawString(MSG.b.c_str(), MSG.b.size());
                //pkt.putRawString(MSG.a.c_str(), MSG.a.size());
                pkt.putLongString(MSG.b);
                pkt.putLongString(MSG.a);
            }
            //Send
            m_server->Send(&pkt);
        }

        // Clear buffered_messages
        for (auto &buffered_message : buffered_messages) {
            delete buffered_message.second;
        }
    }
    void acquiredata() {
        ActiveObjectMessage aom(0);
        for(;;) {
            //verbosestream << "getting data" << std::endl;
            if (!m_env->getActiveObjectMessage(&aom)) //m_env must had a locker FIXME: Got it
                break;
            //verbosestream << "not getting data" << std::endl;
            std::vector<ActiveObjectMessage>* message_list = nullptr;
            auto n = buffered_messages.find(aom.id);
            if (n == buffered_messages.end()) {
                message_list = new std::vector<ActiveObjectMessage>;
                buffered_messages[aom.id] = message_list;
            } else {
                message_list = n->second;
            }
            message_list->push_back(std::move(aom));
        }
    }
    void mainf() {
        m_server->m_clients.lock();
        const RemoteClientMap &clients = m_server->m_clients.getClientList();
        // Route data to every client
        std::string reliable_data, unreliable_data;
        for (const auto &client_it : clients) {

            reliable_data.clear();
            unreliable_data.clear();
            RemoteClient *client = client_it.second;
            PlayerSAO *player = m_server->getPlayerSAO(client->peer_id);
            RemotePlayer *p = m_env->getPlayer(client->peer_id);
            //verbosestream << "a" << std::endl;
            if (!p || p->to_other_server)
                continue;

            // Go through all objects in message buffer
            for (const auto &buffered_message : buffered_messages) {

                // If object does not exist or is not known by client, skip it
                u16 id = buffered_message.first;
                //verbosestream << id << std::endl;
                ServerActiveObject *sao = m_env->getActiveObject(id);
                
               /* if ( client->m_known_objects.find(id) == client->m_known_objects.end())
                    verbosestream << "affirmative: " << id << ", " << client->peer_id << std::endl;
                else
                    verbosestream << "huh" << std::endl;

                if (!sao)
                    verbosestream << "no sao for " << id << ", " <<client->peer_id<<std::endl;
*/
                if (!sao || client->m_known_objects.find(id) == client->m_known_objects.end())
                    continue;

                //warningstream << "SENT: " << id<< ", " << p->getPeerId() << "," << p->getName() << std::endl;

                // Get message list of object
                std::vector<ActiveObjectMessage>* list = buffered_message.second;
                // Go through every message
                for (const ActiveObjectMessage &aom : *list) {
                    // Send position updates to players who do not see the attachment
                    if (aom.datastring[0] == AO_CMD_UPDATE_POSITION) {
                        if (sao->getId() == player->getId())
                            continue;

                        // Do not send position updates for attached players
                        // as long the parent is known to the client
                        ServerActiveObject *parent = sao->getParent();
                        if (parent && client->m_known_objects.find(parent->getId()) !=
                            client->m_known_objects.end())
                            continue;
                    }

                    // Add full new data to appropriate buffer
                    std::string &buffer = aom.reliable ? reliable_data : unreliable_data;
                    char idbuf[2];
                    writeU16((u8*) idbuf, aom.id);
                    // u16 id
                    // std::string data
                    //verbosestream << FUNCTION_NAME << ": Sending to " << client->peer_id << " AOM: " << idbuf << aom.datastring << std::endl;
                    buffer.append(idbuf, sizeof(idbuf));
                    if (client->net_proto_version >= 37 ||
                        aom.legacystring.empty())
                        buffer.append(serializeString16(aom.datastring));
                    else
                        buffer.append(serializeString16(aom.legacystring));
                }
            }
            /*
             r eliable_data and* unreliable_data are now ready.
             Send them.
             */
            if (!reliable_data.empty()) {
                m_server->SendActiveObjectMessages(client->peer_id, reliable_data);
            }

            if (!unreliable_data.empty()) {
                m_server->SendActiveObjectMessages(client->peer_id, unreliable_data, false);
            }
        }
        // Clear buffered_messages
        for (auto &buffered_message : buffered_messages) {
            delete buffered_message.second;
        }

        buffered_messages.clear();
        m_server->m_clients.unlock();
    }
private:
    SentNetworkThread *SNT = nullptr;
    ServerEnvironment *m_env = nullptr;
    bool *s = nullptr;
    std::unordered_map<uint16_t, std::vector<ActiveObjectMessage>*> buffered_messages;
    void (*Send)(NetworkPacket*, Server*);
    Server *m_server = nullptr;
};

// S E R V E R

using namespace std;

Server::Server(const string &path, const SubgameSpec &GS, bool ssm, Address addr, bool d):
    MAINSERVEURE(std::make_shared<con::Connection>(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, m_bind_addr.isIPv6(), this)),
    m_async_globals_data(""),
    m_bind_addr(addr),
    m_path_world(path),
    m_gamespec(GS),
    m_async_fatal_error(""),
    m_con(std::make_shared<con::Connection>(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, m_bind_addr.isIPv6(), this)),
    m_clients(m_con),
    m_itemdef(createItemDefManager()),
    m_nodedef(createNodeDefManager()),
    m_craftdef(createCraftDefManager())
{
    //I must delete this later...
    m_simple_singleplayer_mode = false;
    m_dedicated = true;
    //Server
    if (m_path_world.empty())
        throw ServerError("Supplied empty world path");
    //Counters
    m_metrics_backend = std::unique_ptr<MetricsBackend>(new MetricsBackend());
    m_timeofday_gauge = m_metrics_backend->addGauge("minetest_core_timeofday", "Time of day value");
    m_lag_gauge = m_metrics_backend->addGauge("minetest_core_latency", "Latency value (in seconds)");
    m_aom_buffer_counter = m_metrics_backend->addCounter("minetest_core_aom_generated_count", "Number of active object messages generated");
    m_packet_recv_counter = m_metrics_backend->addCounter("minetest_core_server_packet_recv", "Processable packets received");
    m_packet_recv_processed_counter = m_metrics_backend->addCounter( "minetest_core_server_packet_recv_processed",
                                                                     "Valid received packets processed");
    //Special objects
    ServersNetworkObject = new ServerNetworkEngine(m_env, this, m_path_world);
    NetCR = new NetworkCompressor(this);
    if (SocketConn) {
        warningstream << "Deleting networks of server-slave as it will don't be used" << std::endl;
        m_con.reset();
        MAINSERVEURE.reset();
    }
}

/*
 * infostream << "Server: Kicking players" << std::endl;
 s td::string kick_msg; *
 bool reconnect = false;
 if (isShutdownRequested()) {
     reconnect = m_shutdown_state.should_reconnect;
     kick_msg = m_shutdown_state.message;
     }
     if (kick_msg.empty()) {
         kick_msg = g_settings->get("kick_msg_shutdown");
         }
         m_env->saveLoadedPlayers(true);
         m_env->kickAllPlayers(SERVER_ACCESSDENIED_SHUTDOWN,
         kick_msg, reconnect);
 * */

using NTH = void*(*)(void);

Server::~Server() {
    //Announce the shutdown
    SendChatMessage(PEER_ID_INEXISTENT, ChatMessage(CHATMESSAGE_TYPE_ANNOUNCE, L"# Server Shutting down"));
    m_powering_down = true;
    //Disconnect players
    DisconnectAllPlayers();
    ServersNetworkObject->PoweroffServers();
    //Execute hooks
    actionstream << "Executing shutdown hooks" << endl;
    /*try { // Bye...
        m_script->on_shutdown();
    } catch (ModError &e) {
        errorstream << "ModError: " << e.what() << endl;
    }*/
    (reinterpret_cast<NTH>(AddonsCallbacks[CALLBACK_ON_SHUTDOWN]))();
    //Stop threads
    if (m_emerge)
        m_emerge->stopThreads();
    m_env->saveMeta();
    m_env->saveLoadedPlayers();
    if (m_netthr) {
        m_netthr->stop();
    }
    NetCR->stop();
    delete NetCR;
    delete m_emerge;
    delete m_env;
    delete m_banmanager;
    delete m_itemdef;
    delete m_nodedef;
    delete m_craftdef;
    delete m_game_settings;
    delete SSE;
    delete SNT;
    delete m_netthr;
    delete m_mapupdate;
    while (!m_unsent_map_edit_queue.empty()) {
        delete m_unsent_map_edit_queue.pop();
    }
    if (m_thread) {
        stop();
        delete m_thread;
    }
}

//This was made JUST to redirect correctly the data to players, without delay.
/*
 * Structure:
 * Server + SNE = Servers with steroids..... I mean... server with multiple servers
 * [m_con] Receives the data of the players
 * [NetworkThread] Checks the data
 * [Server]: Gets the data if it was for the main server
 * [SNE]: Gets the data if it was for the slave servers
 */

//Spectacle BEGINS!

void Server::init() {
    actionstream << "Server created, INFO: \n"
    << "World: " << m_path_world << "\n"
    << "Addr: " << m_bind_addr.serializeString()
    << endl;
    ServInternal *SI = new ServInternal(this);
    m_thread = SI;
    m_netthr = nullptr;
    // Settings
    m_game_settings = Settings::createLayer(SL_GAME);
    // Create world if it doesn't exist
    try {
        loadGameConfAndInitWorld(m_path_world, fs::GetFilenameFromPath(m_path_world.c_str()), m_gamespec, false);
    } catch (const BaseException &e) {
        throw ServerError(std::string("Failed to initialize world: ") + e.what());
    }
    // BanManager would be useless on a slave server
    if (!ServersNetworkObject->AreSlave) {
        // Create ban manager
        std::string ban_path = m_path_world + DIR_DELIM "ipban.txt";
        m_banmanager = new BanManager(ban_path);
    }
    
    actionstream << "Initializing addons set.." << std::endl;
    sAddons = new servAddons(this);
    
    // Init builtin addon.
    try {
        sAddons->loadAddon(porting::path_share + "/builtin.so");
    } catch (std::runtime_error &e) {
        errorstream << "Could'nt load builtin tools! But running anyways." << std::endl;
        errorstream << "std::runtime_error: " << e.what() << std::endl;
    }
    
    sAddons->initializeSet(m_path_world+"/addons");
    sAddons->setReady();
    actionstream << "Total addons initialized: " << sAddons->getAddonsList().size() << std::endl;
    
    // Mods
    //m_modmgr = std::unique_ptr<ServerModManager>(new ServerModManager(m_path_world));
    //std::vector<ModSpec> unsatisfied_mods = m_modmgr->getUnsatisfiedMods();
    // complain about mods with unsatisfied dependencies
    //if (!m_modmgr->isConsistent()) {
    //    m_modmgr->printUnsatisfiedModsError();
    //}
    //lock environment
    MutexAutoLock envlock(m_env_mutex);
    // Create the Map (loads map_meta.txt, overriding configured mapgen params)

    //Map engine should run on a SEPARATE variable, as we will use multiple maps at once

    //Create startup map folder
    {
        fs::CreateAllDirs(m_path_world+"/main/");
    }

    Maps[0] = new HyperMap();
    m_emerge = new EmergeManager(this, Maps[0]);

    ServerMap *servermap = new ServerMap(m_path_world+"/main/", this, m_emerge, m_metrics_backend.get());
    m_startup_server_map = servermap;
    Maps[0]->m_map = servermap;
    Maps[0]->m_id = 0;
    Maps[0]->m_name = std::string("Main - Overworld");

    // Create emerge manager

    Maps[0]->m_emerge = m_emerge;

    // Scripts
    actionstream << "Initializing scripts" << endl;
    m_inventory_mgr = std::unique_ptr<ServerInventoryManager>(new ServerInventoryManager());
    //Compatibility feature of MultiCraft
    std::string player_models = g_settings->get("compat_player_model");
    player_models.erase(std::remove_if(player_models.begin(), player_models.end(), static_cast<int(*)(int)>(&std::isspace)), player_models.end());
    if (player_models.empty() || isSingleplayer())
        FATAL_ERROR_IF(!m_compat_player_models.empty(), "Compat player models list not empty");
    else
        m_compat_player_models = str_split(player_models, ',');
    fillMediaCache();
    if (!ServersNetworkObject->AreSlave)
        m_nodedef->updateAliases(m_itemdef);
    std::vector<std::string> paths;
    fs::GetRecursiveDirs(paths, g_settings->get("texture_path"));
    fs::GetRecursiveDirs(paths, m_gamespec.path + DIR_DELIM + "textures");
    for (const std::string &path : paths) {
        TextureOverrideSource override_source(path + DIR_DELIM + "override.txt");
        m_nodedef->applyTextureOverrides(override_source.getNodeTileOverrides());
        m_itemdef->applyTextureOverrides(override_source.getItemTextureOverrides());
    }
    if (!ServersNetworkObject->AreSlave) {
        m_nodedef->setNodeRegistrationStatus(true);
        // Perform pending node name resolutions
        m_nodedef->runNodeResolveCallbacks();
        // unmap node names in cross-references
        m_nodedef->resolveCrossrefs();
        // init the recipe hashes to speed up crafting
        m_craftdef->initHashes(this);
        //m_emerge->initMapgens(m_startup_server_map->getMapgenParams());
    }
    // Environment
    m_env = new ServerEnvironment(servermap, nullptr, this, m_path_world, &Maps);
    m_inventory_mgr->setEnv(m_env);
    m_clients.setEnv(m_env);
    if (!servermap->settings_mgr.makeMapgenParams())
        FATAL_ERROR("Couldn't create any mapgen type");
    // Initialize mapgens
    if (!ServersNetworkObject->AreSlave)
        m_emerge->initMapgens(servermap->getMapgenParams());
    servermap->addEventReceiver(this);
    try {
        m_env->loadMeta();
    } catch (SerializationError &e) {
        warningstream << "Environment metadata is corrupted: " << e.what() << std::endl;
        warningstream << "Loading the default instead" << std::endl;
        m_env->loadDefaultMeta();
    }
    m_liquid_transform_every = g_settings->getFloat("liquid_update");
    m_max_chatmessage_length = g_settings->getU16("chat_message_max_size");
    m_csm_restriction_flags = g_settings->getU64("csm_restriction_flags");
    m_csm_restriction_noderange = g_settings->getU32("csm_restriction_noderange");
    ServersNetworkObject->SetEnv(m_env);
    if (ServersNetworkObject->AreSlave && !SocketConn)
        NetCR->start();
    if (!ServersNetworkObject->AreSlave) {
        SentNetworkThread *m_SNT = new SentNetworkThread(this);
        SNT = m_SNT;
        m_SNT->start();
    }
    AreSlave = new bool(true);
    *AreSlave = ServersNetworkObject->AreSlave;
    //errorstream << (AreSlave ? "true" : "false") << std::endl;
    //errorstream << (ServersNetworkObject->AreSlave ? "true" : "false") << std::endl;
    ThisServID = ServersNetworkObject->QueryThisServerID();
    TAOM = new ThreadingAOM(this, m_env, &ServersNetworkObject->AreSlave, nullptr);
    if (!ServersNetworkObject->AreSlave)
        m_netthr = new NetworkThread(this);
    errorstream<<"ended"<<std::endl;
}

void Server::start() {
    init();
    if (!ServersNetworkObject->AreSlave) {
        m_thread->stop();
        m_netthr->stop();
    }
    m_thread->start();
    std::string flags = ""; //Used to know which protocol the server uses
    // Initialize connection
    actionstream<<"Initializing connections................"<<std::endl;
    if (!SocketConn && ServersNetworkObject->AreSlave) {
        verbosestream<<"Classic subServer connection found"<<std::endl;
        m_con->SetTimeoutMs(30);
        m_con->Serve(m_bind_addr);
        flags += " SlaveServer::ClassicConnection ";
    } else if (ServersNetworkObject->AreSlave && SocketConn) {
        verbosestream<<"Socket connection found"<<std::endl;
        m_netthr = new NetworkThread(this);
        m_netthr->start();
        flags += " SlaveServer::SocketConn[Fast] ";
    } else if (!ServersNetworkObject->AreSlave) {
        verbosestream<<"Main server connection found" << std::endl;
        m_con->SetTimeoutMs(30);
        m_con->Serve(m_bind_addr);
        m_netthr->start();
        flags += " MainServer {" + m_bind_addr.serializeString() + "}";
    }
    actionstream << "Server on with flags:" << flags << std::endl;
    if (ServersNetworkObject->AreSlave)
        sendDefinitions();
    // Server Sector SCRIPT
    ServSectorEnv *_SSE = new ServSectorEnv(this);
    SSE = _SSE;
    SSE->start();
    ServersNetworkObject->OnInitServer();
    m_mapupdate = new MapUpdate(this, m_env);
    m_mapupdate->start();
    TAOM->start();
    actionstream << "▖  ▖▘    ▄▖▗       "
    << "\n▛▖▞▌▌▛▌█▌▚ ▜▘▀▌▛▘▛▘"
    << "\n▌▝ ▌▌▌▌▙▖▄▌▐▖█▌▌ ▄▌" << std::endl;
}

void Server::stop()
{
    actionstream << "Server: Stopping and waiting threads" << endl;
    m_thread->stop();
    m_thread->wait();
    m_mapupdate->stop();
    m_mapupdate->wait();
    if (m_netthr != nullptr) {
        m_netthr->stop();
        m_netthr->wait();
    }
    if (SNT != nullptr) {
        SNT->stop();
        SNT->wait();
    }
    SSE->stop();
    SSE->wait();
    ServersNetworkObject->StopNkillThreads();
    actionstream << "Server: Threads stopped" << std::endl;
}

void Server::step(float dtime) {
    if (dtime > 2.0)
        dtime = 2.0;
    {
        MutexAutoLock lock(m_step_dtime_mutex);
        m_step_dtime += dtime;
    }
    // Power down if error appeared on a thread
    std::string async_err = m_async_fatal_error.get();
    if (!async_err.empty()) {
        errorstream << async_err << std::endl;
        async_err.clear();
        //DisconnectAllPlayers(true);
        //ServersNetworkObject->PoweroffServers();
        //throw ServerError("AsyncErr: " + async_err);
    }
}

//NOTE: The functions, which updates player clients will be modified, lol....

//BEGIN

// Server Updates

//Only updates dtime, uptime, speed of time, days, etc...

//Only for main server [Thread must be called Internal]
void Server::AsyncRunStepMain(bool initial_step) {
    float dtime;
    {
        MutexAutoLock lock1(m_step_dtime_mutex);
        dtime = m_step_dtime;
    }
    if((dtime < 0.001) && !initial_step)
        return;
    SendBlocks(dtime);
    {
        MutexAutoLock lock1(m_step_dtime_mutex);
        m_step_dtime -= dtime;
        //m_uptime_counter->increment(dtime);
    }
    handlePeerChanges();
    m_env->setTimeOfDaySpeed(g_settings->getFloat("time_speed"));
    m_time_of_day_send_timer -= dtime;
    if (m_time_of_day_send_timer < 0.0) {
        m_time_of_day_send_timer = g_settings->getFloat("time_send_interval");
        u16 time = m_env->getTimeOfDay();
        float time_speed = g_settings->getFloat("time_speed");
        SendTimeOfDay(PEER_ID_INEXISTENT, time, time_speed);
        //m_timeofday_gauge->set(time);
    }
    {
        MutexAutoLock lock(m_env_mutex);
        // Figure out and report maximum lag to environment
        float max_lag = m_env->getMaxLagEstimate();
        max_lag *= 0.9998; // Decrease slowly (about half per 5 minutes)
        if(dtime > max_lag) {
            if(dtime > 0.1 && dtime > max_lag * 2.0) {
                warningstream << "ServerMain: Maximum lag peaked to " << dtime << " s" << std::endl;
                max_lag = dtime;
            }
        }
        m_env->reportMaxLagEstimate(max_lag);
    }
    m_clients.step(dtime);
    if (m_lag_gauge->get() > dtime) {
        m_lag_gauge->decrement(dtime/3);
    } else {
        m_lag_gauge->increment(dtime/3);
    }
    // send masterserver announce
    {
        float &counter = m_masterserver_timer;
        if (!isSingleplayer() && (!counter || counter >= 60.0) &&
            g_settings->getBool("server_announce")) {
            ServerList::sendAnnounce(counter ? ServerList::AA_UPDATE :
            ServerList::AA_START,
            m_bind_addr.getPort(),
                                     m_clients.getPlayerNames(),
                                     m_uptime_counter->get(),
                                     m_env->getGameTime(),
                                     m_lag_gauge->get(),
                                     m_gamespec.id,
                                     Mapgen::getMapgenName(m_emerge->mgparams->mgtype),
                                     std::vector<ModSpec>({}),
                                     true);
            counter = 0.01;
            }
            counter += dtime;
    }
    m_clients.lock();
    const RemoteClientMap &clients = m_clients.getClientList();
    //m_player_gauge->set(clients.size());
    for (const auto &client_it : clients) {
        RemoteClient *client = client_it.second;
        if (client->getState() < CS_DefinitionsSent)
            continue;
        RemotePlayer *player = m_env->getPlayer(client->peer_id);
        // This can happen if the client times out somehow or he are uninitialized
        if (!player)
            continue;
        if (player->to_other_server) //Means the player are on other server...
            continue;
        PlayerSAO *playersao = getPlayerSAO(client->peer_id);
        if (!playersao)
            continue;
        SendActiveObjectRemoveAdd(client, playersao);
    }
    m_clients.unlock();
    /*if (m_mod_storage_save_timer <= 0.0f) {
        m_mod_storage_save_timer = g_settings->getFloat("server_map_save_interval");
        int n = 0;
        for (std::unordered_map<std::string, ModMetadata *>::const_iterator
            it = m_mod_storages.begin(); it != m_mod_storages.end(); ++it) {
            if (it->second->isModified()) {
                it->second->save(getModStoragePath());
                n++;
            }
        }
        if (n > 0)
            actionstream << "Saved " << n << " modified mod storages." << std::endl;
    }*/

    {
        float &counter = m_emergethread_trigger_timer;
        counter += dtime;
        if (counter >= 2.0) {
            counter = 0.0;
            m_emerge->startThreads();
        }
    }

    //Map edit events

    //(moved to a diff thread)
    // i ♥️ threads
    QueuedPlayersToInitialize.unlock();
    PlayersToInitialize_MTX.lock();
    while (!PlayersToInitialize.empty()) {
        session_t p = PlayersToInitialize.front();
        InitializePlayer(p);
        PlayersToInitialize.pop_front();
    }
    PlayersToInitialize_MTX.unlock();
    m_shutdown_state.tick(dtime, this);
}

void Server::AsyncRunStepSlave(bool initial_step) {
    float dtime;
    {
        MutexAutoLock lock1(m_step_dtime_mutex);
        dtime = m_step_dtime;
    }
    if((dtime < 0.001) && !initial_step)
        return;
    SendBlocks(dtime);
    {
        MutexAutoLock lock1(m_step_dtime_mutex);
        m_step_dtime -= dtime;
        //m_uptime_counter->increment(dtime);
    }
    m_env->setTimeOfDaySpeed(g_settings->getFloat("time_speed"));
    m_time_of_day_send_timer -= dtime;
    if (m_time_of_day_send_timer < 0.0) {
        m_time_of_day_send_timer = g_settings->getFloat("time_send_interval");
        u16 time = m_env->getTimeOfDay();
        float time_speed = g_settings->getFloat("time_speed");
        SendTimeOfDay(PEER_ID_INEXISTENT, time, time_speed);
        //m_timeofday_gauge->set(time);
    }
    {
        MutexAutoLock lock(m_env_mutex);
        // Figure out and report maximum lag to environment
        float max_lag = m_env->getMaxLagEstimate();
        max_lag *= 0.9998; // Decrease slowly (about half per 5 minutes)
        if(dtime > max_lag) {
            if(dtime > 0.1 && dtime > max_lag * 2.0)
                warningstream << "Server: Maximum lag peaked to " << dtime << " s" << std::endl;
            max_lag = dtime;
        }
        m_env->reportMaxLagEstimate(max_lag);
    }
    static const float map_timer_and_unload_dtime = 2.92;
    if (m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime)) {
        MutexAutoLock lock(m_env_mutex);
        // Run Map's timers and unload unused data
        m_env->getMap().timerUpdate(map_timer_and_unload_dtime, g_settings->getFloat("server_unload_unused_data_timeout"), U32_MAX);
    }
    if (m_lag_gauge->get() > dtime) {
        m_lag_gauge->decrement(dtime/3);
    } else {
        m_lag_gauge->increment(dtime/3);
    }
    //m_player_gauge->set(ClientDataTable.getSize());
    ClientDataTable.Lock();
    for (auto it = ClientDataTable.GetRawMap()->begin(); it != ClientDataTable.GetRawMap()->end(); ++it) {
        ClientDataHelper *client = it->second;
        if (!m_env->getPlayer(client->GetPlayerID_()))
            continue;
        PlayerSAO *playersao = getPlayerSAO(client->GetPlayerID_());
        if (!playersao)
            continue;
        SendActiveObjectRemoveAdd_SLAVE(client, playersao);
    }
    ClientDataTable.unLock();
    //m_mod_storage_save_timer -= dtime;
    /*if (m_mod_storage_save_timer <= 0.0f) {
        m_mod_storage_save_timer = g_settings->getFloat("server_map_save_interval");
        int n = 0;
        for (std::unordered_map<std::string, ModMetadata *>::const_iterator
            it = m_mod_storages.begin(); it != m_mod_storages.end(); ++it) {
            if (it->second->isModified()) {
                it->second->save(getModStoragePath());
                n++;
            }
            }
            if (n > 0)
                actionstream << "Saved " << n << " modified mod storages." << std::endl;
    }*/
    {
        float &counter = m_emergethread_trigger_timer;
        counter += dtime;
        if (counter >= 2.0) {
            counter = 0.0;
            m_emerge->startThreads();
        }
    }
    {
        float &counter = m_savemap_timer;
        counter += dtime;
        static thread_local const float save_interval =
        g_settings->getFloat("server_map_save_interval");
        if (counter >= save_interval) {
            counter = 0.0;
            MutexAutoLock lock(m_env_mutex);
            // Save ban file
            if (m_banmanager->isModified()) {
                m_banmanager->save();
            }
            // Save changed parts of map
            m_env->getMap().save(MOD_STATE_WRITE_NEEDED);
            // Save players
            m_env->saveLoadedPlayers();
            // Save environment metadata
            m_env->saveMeta();
        }
    }
    QueuedPlayersToInitialize.lock();
    while (!ToInitialize.empty()) {
        PlayerInitializerSLV p = std::move(ToInitialize.front());
        ToInitialize.pop_front();
        s64 last_login;
        //m_script->getAuth(p.NAME, nullptr, nullptr, &last_login);
        //m_script->on_joinplayer(p.SAO, last_login);
    }
}

//END of AsyncRunStep

//BEGIN internal functions

void Server::printToConsoleOnly(const std::string &text)
{
    std::cout << text << std::endl;
}

void Server::DisconnectAllPlayers(bool crashed) {
    if (ServersNetworkObject->AreSlave) {
        //FIXME: Must update protocol to set as "Server failed"
        NetworkPacket pkt(0x65, 0);
        pkt << (u8)ServersNetworkObject->QueryThisServerID();
        //FIXME: pkt << (u8)crashed;
        ClientDataTable.Lock();
        for (auto it = ClientDataTable.GetRawMap()->begin(); it != ClientDataTable.GetRawMap()->end(); ++it) {
            pkt << it->second->GetPlayerID_() << (u8) 0 << (u8) 0;
            Send(&pkt);
        }
        ClientDataTable.unLock();
        return;
    } else {
        m_env->kickAllPlayers(SERVER_ACCESSDENIED_CRASH, g_settings->get("kick_msg_crash"), g_settings->getBool("ask_reconnect_on_crash"));
    }
}

void Server::QueueSendTo(session_t peer, NetworkPacket pkt, bool reliable) {
    pkt.setReliableOpt(reliable);
    PacketsDequeMTX.lock();
    pkt.setPeerID(peer);
    PacketsDeque.push_back(pkt);
    PacketsDequeMTX.unlock();
}

u16 Server::getProtocolOfThisPeer(session_t PeerID) {
    m_clients.lock();
    try {
        RemoteClient* client = m_clients.lockedGetClientNoEx(PeerID);
        m_clients.unlock();
        if (client) {
            return client->net_proto_version;
        } else {
            return 0;
        }
    } catch (std::exception &e) {
        m_clients.unlock();
        throw;
    }
    m_clients.unlock();
}


float Server::getRecommendedSendInterval() {
    return m_env->getSendRecommendedInterval();
}

bool Server::equalsPlayerIDwithPeerID(u16 PlayerID, session_t PeerID) {
    RemotePlayer *player = m_env->getPlayerByID(PlayerID);
    if (!player)
        return false;
    if (player->getPeerId() == PeerID)
        return true;
    return false;
}

void Server::MakeThisASlaveServer(u16 id) {
    ServersNetworkObject->AreSlave = true;
    //Make this server act like a client for the proxy; Only for sending contents, receiving contents are in Server::Receive()
    if (!ServersNetworkObject->ProxyAddressAdded)
        FATAL_ERROR("Invalid Address for proxy, may you have don't set the proxy address?");
    MAINSERVEURE->SetTimeoutMs(0);
    MAINSERVEURE->Connect(ServersNetworkObject->ProxyAddress); //Element ProxyAddress are defined in mods
}
void Server::HandleIDforServer(NetworkPacket *pkt) {
    u16 ID__;
    u8 null;
    *pkt >> null >> ID__;
    ServersNetworkObject->SetTSID(ID__);
    actionstream << "Received command from a proxy serv: 0x66, setting THIS server as an proxy, ID: " << ID__ << std::endl;
}

void Server::QueueOnJoinPlayer(PlayerSAO *sao, const char *name) {
    QueuedPlayersToInitialize.lock();
    ToInitialize.emplace_back();
    auto &queuer = ToInitialize.back();
    queuer.SAO = sao;
    queuer.NAME = name;
    QueuedPlayersToInitialize.unlock();
}

void Server::sendDefinitions() {
    warningstream << "Sending definitions!" << std::endl;
    u16 s_id = ServersNetworkObject->QueryThisServerID();
    //Send nodedef
    NetworkPacket pkt(0x70, 1);
    pkt << (u8)s_id;

    //standard sending (server->client) but proxy

    std::ostringstream osx1(std::ios::binary);
    m_itemdef->serialize(osx1, 38);
    std::ostringstream osx2(std::ios::binary);
    compressZlib(osx1.str(), osx2);
    pkt.putLongString(osx2.str());

    std::ostringstream tmp_os(std::ios::binary);
    m_nodedef->serialize(tmp_os, 38);
    std::ostringstream tmp_os2(std::ios::binary);
    compressZlib(tmp_os.str(), tmp_os2);
    pkt.putLongString(tmp_os2.str());
    Send(&pkt);
}

bool Server::ExistsID(u16 ID) {
    return Players.Has(ID);
}

void Server::QueuePlayerToInitialize(session_t p) {
    PlayersToInitialize_MTX.lock();
    PlayersToInitialize.push_back(p);
    PlayersToInitialize_MTX.unlock();
}

u16 Server::GetRandomIDforPlayer() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<u16> dist(4500, UINT16_MAX); // Pretty sure the player limit are 200......
    return dist(gen);
}

//END internal functions

//BEGIN ServerSoundParams

v3f ServerSoundParams::getPos(ServerEnvironment *env, bool *pos_exists) const
{
    if(pos_exists) *pos_exists = false;
    switch(type){
        case SSP_LOCAL:
            return v3f(0,0,0);
        case SSP_POSITIONAL:
            if(pos_exists) *pos_exists = true;
            return pos;
        case SSP_OBJECT: {
            if(object == 0)
                return v3f(0,0,0);
            ServerActiveObject *sao = env->getActiveObject(object);
            if(!sao)
                return v3f(0,0,0);
            if(pos_exists) *pos_exists = true;
            return sao->getBasePosition(); }
    }
    return v3f(0,0,0);
}

//END ServerSoundParams

//BEGIN ServerConsoleManagement

//NOTE: Add an console chat, without external libraries, using std::cin, std::stdout, and others..... . . . .

//END ServerConsoleManagement

//BEGIN DEDICATED_SERVER_LOOP

void dedicated_server_loop(Server &server, bool &kill)
{
    verbosestream<<"dedicated_server_loop()"<<std::endl;
    static thread_local const float steplen = 0.02;

    /*
     * The dedicated server loop only does time-keeping (in Server::step) and
     * provides a way to main.cpp to kill the server externally (bool &kill).
     */

    for(;;) {
        // This is kind of a hack but can be done like this
        // because server.step() is very light
        sleep_ms((int)(steplen*1000.0));
        server.step(steplen);

        if (server.isShutdownRequested() || kill)
            break;
    }

    infostream << "Dedicated server quitting" << std::endl;
    #if USE_CURL
    if (g_settings->getBool("server_announce"))
        ServerList::sendAnnounce(ServerList::AA_DELETE,
                                 server.m_bind_addr.getPort());
        #endif
}

//END DEDICATED_SERVER_LOOP

void Server::setTimeOfDay(u32 time)
{
    m_env->setTimeOfDay(time);
    m_time_of_day_send_timer = 0;
}

void Server::onMapEditEvent(const MapEditEvent &event)
{
    if (m_ignore_map_edit_events_area.contains(event.getArea()))
        return;

    m_mapupdate->add(new MapEditEvent(event));
}

void Server::SetBlocksNotSent(std::map<v3s16, MapBlock *>& block)
{
    std::vector<session_t> clients = m_clients.getClientIDs();
    m_clients.lock();
    // Set the modified blocks unsent for all the clients
    for (const session_t client_id : clients) {
        if (RemoteClient *client = m_clients.lockedGetClientNoEx(client_id))
            client->SetBlocksNotSent(block);
    }
    m_clients.unlock();
}

void Server::reportPrivsModified(const std::string &name)
{
    if (!ServersNetworkObject->AreSlave) {
        if (name.empty()) {
            std::vector<session_t> clients = m_clients.getClientIDs();
            for (const session_t client_id : clients) {
                RemotePlayer *player = m_env->getPlayer(client_id);
                reportPrivsModified(player->getName());
            }
        } else {
            RemotePlayer *player = m_env->getPlayer(name.c_str());
            if (!player)
                return;
            SendPlayerPrivileges(player->getPeerId());
            PlayerSAO *sao = player->getPlayerSAO();
            if(!sao)
                return;
            sao->updatePrivileges(
                getPlayerEffectivePrivs(name),
                                  isSingleplayer());
        }
    } else {
        if (name.empty()) {
            ClientDataTable.Lock();
            for (auto it = ClientDataTable.GetRawMap()->begin(); it != ClientDataTable.GetRawMap()->end(); ++it) {
                RemotePlayer *player = m_env->getPlayer(it->first);
                reportPrivsModified(player->getName());
            }
            ClientDataTable.unLock();
        } else {
            RemotePlayer *player = m_env->getPlayer(name.c_str());
            if (!player)
                return;
            SendPlayerPrivileges(player->player_id);
            PlayerSAO *sao = player->getPlayerSAO();
            if(!sao)
                return;
            sao->updatePrivileges(getPlayerEffectivePrivs(name), isSingleplayer());
        }
    }
}

Map &Server::getMap() {
    return m_env->getMap(0);
}




























