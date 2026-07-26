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


#include "../player.h"
#include "../script/scripting_server.h"
#include "../slave_helpers.h"
#include "../server.h"
#include "../remoteplayer.h"
#include "../log.h"
#include "../server/player_sao.h"
#include "../ServerNetworkEngine.h"
#include "NetworkEngine.h"
#include "../addons/cblks_def.hpp"
#include <cstdint>
#include "map.h"

//BEGIN RemoveAddPlayer

PlayerSAO *Server::StageTwoClientInit(uint16_t _ID) {
    std::string pname;
    PlayerSAO *ps = nullptr;
    RemotePlayer *p = nullptr;
    Address addr;
    errorstream << (AreSlave ? "true" : "false") << std::endl;
    if (!ServersNetworkObject->AreSlave) {
        verbosestream << FUNCTION_NAME << ": Player prejoining: " << _ID << std::endl;
        session_t peer_id = _ID;
        m_clients.lock();
        try {
            RemoteClient *c = m_clients.lockedGetClientNoEx(peer_id, CS_InitDone);
            if (c) {
                pname = c->getName();
                ps = emergePlayer(pname.c_str(), _ID, c->net_proto_version);
            }
        } catch (std::exception &e) {
            m_clients.unlock();
        }
        m_clients.unlock();

        p = m_env->getPlayer(pname.c_str());

        if (!ps || !p) {
            if (p && p->getPeerId() != PEER_ID_INEXISTENT) {
                warningstream << "Failed to emerge player (" << pname << "); Player already joined" << std::endl;
                DenyAccess(peer_id, SERVER_ACCESSDENIED_ALREADY_CONNECTED);
            } else {
                errorstream << "Failed to emerge player (" << pname << ")" << std::endl;
            }
            return nullptr;
        }

        //Just addressing the codes
        PlayerInternalInfo *pl = new PlayerInternalInfo();
        pl->PlayerID = p->getPlayerID();
        pl->ServerID = 0;
        pl->PeerID = _ID;
        pl->PlayingOnServ = false;

        if (PeerIdPlayers.Has(_ID)) {
            if (PeerIdPlayers.Get(_ID) != nullptr) {
                delete PeerIdPlayers.Get(_ID); //Delete older pointer if it exists, just to refresh
            }
        }

        PeerIdPlayers.Set(_ID, pl);
        
        SlaveServer *_ss = new SlaveServer();
        p->OnServer = _ss;
        p->OnServer->InSlaveServerISPLAYINGHERE = true;
        p->OnServer->IsApplied = false;
        p->OnServer->ID = 65535;
    }

    if (ps->isDead())
        SendDeathscreen(_ID, false, v3f(0,0,0));
    else
        SendPlayerHPOrDie(ps, PlayerHPChangeReason(PlayerHPChangeReason::SET_HP));

    SendPlayerBreath(ps);
    SendMovePlayer(_ID);
    SendPlayerPrivileges(_ID);
    SendPlayerInventoryFormspec(_ID);
    SendInventory(ps, false);

    if (!ServersNetworkObject->AreSlave) {
        addr = getPeerAddress(p->getPeerId());
    }

    {
        std::string identifier = ServersNetworkObject->AreSlave ? std::to_string(p->getPlayerID()) : addr.serializeString();
        actionstream << p->getName() << " [" << identifier << "] joins game. List of players: " << std::endl;
    }

    return ps;
}

void Server::InitializePlayer(session_t pid) {
    RemotePlayer *player = m_env->getPlayer(pid);
    PlayerSAO *playersao = player->getPlayerSAO(); //
    playersao->finalize(player, getPlayerEffectivePrivs(player->getName()));
    SendMovePlayer(pid);
    SendPlayerPrivileges(pid);
    SendPlayerInventoryFormspec(pid);
    SendInventory(playersao, false);
    if (playersao->isDead())
        SendDeathscreen(pid, false, v3f(0,0,0));
    else
        SendPlayerHPOrDie(playersao, PlayerHPChangeReason(PlayerHPChangeReason::SET_HP));
    SendPlayerBreath(playersao);
    const std::vector<std::string> &players = m_clients.getPlayerNames();
    std::vector<std::string> PLIST; //filter players
    for (const std::string &player: players) {
        RemotePlayer *p = m_env->getPlayer(player.c_str());
        if (!p->OnServer->IsApplied) {
            PLIST.push_back(player);
        } else {
            warningstream << "Skipping player " << player << " as it are in other realm" << std::endl;
        }
    }
    NetworkPacket list_pkt(TOCLIENT_UPDATE_PLAYER_LIST, 0);
    list_pkt << (u8) PLAYER_LIST_INIT << (u16) players.size();
    for (std::string player : PLIST) {
        list_pkt << player;
    }
    m_clients.send(pid, 0, &list_pkt, true);
    s64 last_login;
    //m_script->getAuth(playersao->getPlayer()->getName(), nullptr, nullptr, &last_login);
    //m_script->on_joinplayer(playersao, last_login);
    (reinterpret_cast<void*(*)(PlayerSAO*)>(AddonsCallbacks[CALLBACK_ON_JOINPLAYER]))(playersao);
    {
        Address addr = getPeerAddress(player->getPeerId());
        std::string ip_str = addr.serializeString();
        const std::vector<std::string> &names = m_clients.getPlayerNames();
        actionstream << player->getName() << " [" << ip_str << "] joins game. List of players: ";
        for (const std::string &name : names) {
            actionstream << name << " ";
        }
        actionstream << player->getName() <<std::endl;
    }

    PlayerInternalInfo *raw = PeerIdPlayers.Get(player->getPeerId());
    if (raw == NULL) {
        warningstream << "[Internal]: Going to get a unknown player data! Creating new one" << std::endl;
        PlayerInternalInfo *pl = new PlayerInternalInfo();
        pl->PlayerID = player->getPlayerID();
        pl->ServerID = 0;
        pl->PeerID = pid;
        pl->PlayingOnServ = false;

        if (PeerIdPlayers.Has(pid)) {
            if (PeerIdPlayers.Get(pid) != nullptr) {
                delete PeerIdPlayers.Get(pid); //Delete older pointer if it exists, just to refresh
            }
        }

        PeerIdPlayers.Set(pid, pl);
        return;
    }
    raw->PlayingOnServ = false;
    raw->ServerID = 0;
}

PlayerSAO* Server::emergePlayer(const char *name, uint16_t pid, u16 proto_version) {
    RemotePlayer *player = m_env->getPlayer(name);

    //pid = session_t
    //so, uhmm.. the pid might be an peer id or an player id.. which is known if this is a slave or a main server

    if (player && !ServersNetworkObject->AreSlave) {
        warningstream << "Player not bound or already connected!" << std::endl;
        return NULL;
    }

    if (ServersNetworkObject->AreSlave) {
        if (m_env->getPlayer(pid)) {
            warningstream << "Player: Unsupported name; but peer id already exists";
            return NULL;
        }
    }

    //If player not joined, make him join this amazing server
    if (!player) {
        player = new RemotePlayer(name, idef());
    }

    // PSEUDO RANDOM Unsigned Short Int Generator
    u16 preferID;
    bool got_id = false;
    while (!got_id) {
        u16 to_use_rdm = GetRandomIDforPlayer();
        if (!ExistsID(to_use_rdm)) {
            got_id = true;
            preferID = to_use_rdm;
        }
    }

    infostream << FUNCTION_NAME << ": Got PID for player: " << player->getName() << " = " << (ServersNetworkObject->AreSlave ? pid : preferID) << "::" << player->getPeerId() << std::endl;

    //Data parsing, i hate this, this is like doing some documents which the responses equals each others, wtf


    player->setPeerId(pid); //If proxy, always peerid
    player->player_id = ServersNetworkObject->AreSlave ? pid : preferID;

    bool newplayer = false;
    
    // Load player
    PlayerSAO *playersao = m_env->loadPlayer(player, &newplayer, pid, 0); //Map id when loading player for the first time should be 0
    
    if (ServersNetworkObject->AreSlave)
        playersao->setServerState(true);
    playersao->finalize(player, getPlayerEffectivePrivs(player->getName()));
    player->protocol_version = proto_version;

    if (ServersNetworkObject->AreSlave) {
        NetworkPacket _pkt(0x76, 5, pid, 0, ThisServID);
        _pkt << playersao->getId();
        Send(&_pkt);
    }

    //new player => m_script
    if (newplayer) {
        //m_script->on_newplayer(playersao);
        (reinterpret_cast<void*(*)(PlayerSAO*)>(AddonsCallbacks[CALLBACK_ON_NEWPLAYER]))(playersao);
    }


    if (SessionToPlayer.size() < (player->getPeerId()+1))
        SessionToPlayer.resize(player->getPeerId()+1);
    SessionToPlayer[player->getPeerId()] = preferID;

    //Assign again some default values, for maps


    return playersao;
}

//BEGIN SLAVE_SERVER

PlayerSAO* Server::InitClientByMainServer(u16 ID, ClientDataHelper *client) {
    std::string playername = client->getName();
    PlayerSAO *playersao = emergePlayer(playername.c_str(), ID, client->net_proto_version);

    RemotePlayer *player = m_env->getPlayer(playername.c_str());

    // If failed, cancel
    if (!playersao || !player) {
        if (player && player->player_id != 0) {
            actionstream << "Server: Failed to emerge player \"" << playername << "\" (player allocated to an another client)" << std::endl;
        } else {
            errorstream << "Server: " << playername << ": Failed to emerge player" << std::endl;
        }
        return nullptr;
    }

    player->OnServer->InSlaveServerISPLAYINGHERE = true;
    player->OnServer->IsApplied = true;
    player->OnServer->ID = ServersNetworkObject->QueryThisServerID();
    player->player_id = ID;
    player->ID_OF_TRUE_SAO = client->IdInSlave;
    player->setPeerId(ID);

    /*
     *	Send complete position information
     */
    SendMovePlayer(ID);

    // Send privileges
    SendPlayerPrivileges(ID);

    // Send inventory formspec
    SendPlayerInventoryFormspec(ID);

    // Send inventory
    SendInventory(playersao, false);

    // Send HP or death screen
    if (playersao->isDead())
        SendDeathscreen(ID, false, v3f(0,0,0));
    else
        SendPlayerHPOrDie(playersao, PlayerHPChangeReason(PlayerHPChangeReason::SET_HP));

    // Send Breath
    SendPlayerBreath(playersao);

    /*
     *	Print out action
     */
    {
        actionstream << player->getName() << " [" << playername << "|PROXY|ID<" << client->IdInSlave << ">,sID<" << playersao->getId() << ">] joins game." << std::endl;
    }


    if (SessionToPlayer.size() < ID)
        SessionToPlayer.resize(ID);
    SessionToPlayer[ID] = ID;
    std::wstring message = L"MineStars: You're on a subserver. Welcome to the magic realm! :)";
    SendChatMessage(ID, ChatMessage(CHATMESSAGE_TYPE_ANNOUNCE, message));
    ClientDataTable.Set(ID, client);
    //Send to main server to delete the SAO in proxy [SAO are not deleted when sendConnect used, because the client might crash with SIGSEGV <tested>]
    NetworkPacket r_pkt(0x68, 0, ID, 0, ServersNetworkObject->QueryThisServerID());
    Send(&r_pkt);
    return playersao;
}

//END SLAVE_SERVER

void Server::DeleteClient(session_t peer_id, ClientDeletionReason reason) {
    //Peer_id might be P.ID if are an slave
    bool was_in_slave = AreSlave;
    std::wstring message;
    {
        if (ServersNetworkObject->AreSlave) {
            PlayerInternalInfo *found = PeerIdPlayers.Get(peer_id);
            if ((found != nullptr) && found->PlayingOnServ) {
                //Send disconnect to a server
                RemotePlayer *player = m_env->getPlayer(peer_id);
                u16 SrvID = found->ServerID;
                ServersNetworkObject->SendDisconnectONLY(player->player_id, SrvID);
                found->PlayingOnServ = false;
                found->ServerID = NULL;
                was_in_slave = true;
            }
        }
        /*
         *		Clear references to playing sounds
         */
        for (std::unordered_map<s32, ServerPlayingSound>::iterator i = m_playing_sounds.begin(); i != m_playing_sounds.end(); i++) {
            ServerPlayingSound &psound = i->second;
            psound.clients.erase(peer_id);
            if (psound.clients.empty())
                m_playing_sounds.erase(i++);
            else
                ++i;
        }

        // clear formspec info so the next client can't abuse the current state
        m_formspec_state_data.erase(peer_id);

        RemotePlayer *player = m_env->getPlayer(peer_id);

        /* Run scripts and remove from environment */
        if (player) {
            PlayerSAO *playersao = player->getPlayerSAO();
            //NOTE: If the playersao are not found, the player might be playing on a separate server
            if (playersao) {
                playersao->clearChildAttachments();
                playersao->clearParentAttachment();
                // inform connected clients
                const std::string &player_name = player->getName();
                //FIXME: The notice packet should be coded in lua side or haxe side
                /*
                NetworkPacket notice(TOCLIENT_UPDATE_PLAYER_LIST, 0, PEER_ID_INEXISTENT);
                // (u16) 1 + std::string represents a vector serialization representation
                notice << (u8) PLAYER_LIST_REMOVE  << (u16) 1 << player_name;
                m_clients.sendToAll(&notice);
                */
                // run scripts
                //m_script->on_leaveplayer(playersao, reason == CDR_TIMEOUT);
                (reinterpret_cast<void*(*)(PlayerSAO*, ClientDeletionReason)>(AddonsCallbacks[CALLBACK_ON_LEAVEPLAYER]))(playersao, reason);
                playersao->disconnected();
            }
        }

        /*
         *		Print out action
         */
        {
            if (player && reason != CDR_DENY) {
                std::string name = player->getName();
                actionstream << name << " " << (reason == CDR_TIMEOUT ? "times out." : " leaves game.") << std::endl;
            }
        }
        //Delete references
        if (!ServersNetworkObject->AreSlave) {
            MutexAutoLock env_lock(m_env_mutex);
            m_clients.DeleteClient(peer_id);
        } else {
            delete ClientDataTable.Get(peer_id);
            ClientDataTable.Erase(peer_id);
        }
    }

    // Send leave chat message to all remaining clients
    if (!message.empty()) {
        if (!was_in_slave) {
            if (!ServersNetworkObject->AreSlave)
                SendChatMessage(PEER_ID_INEXISTENT, ChatMessage(CHATMESSAGE_TYPE_ANNOUNCE, message));
        }
    }

    //Delete some parts
    //auto it = SessionsStatus.find(peer_id);
    //if (it != SessionsStatus.end())
    //ServersNetworkObject->SendDisconnect(SessionsToU16[peer_id], PlayerInEnvironmentToServer[peer_id]);
    //SessionsStatus.erase(peer_id);

    PlayerInternalInfo *info = PeerIdPlayers.Get(peer_id);
    if (info != nullptr) {
        PeerIdPlayers.Erase(peer_id); //Delete reference
        delete info; //Delete raw data
    }
    if (SessionToPlayer.size() > peer_id)
        SessionToPlayer[peer_id] = false;
}

void Server::OnlyDeleteSAO(session_t peer_id, u16 ID_) {
    //Remove sounds
    for (std::unordered_map<s32, ServerPlayingSound>::iterator i = m_playing_sounds.begin(); i != m_playing_sounds.end();) {
        ServerPlayingSound &psound = i->second;
        psound.clients.erase(peer_id);
        if (psound.clients.empty())
            m_playing_sounds.erase(i++);
        else
            ++i;
    }
    m_formspec_state_data.erase(peer_id);
    RemotePlayer *player = m_env->getPlayer(peer_id);
    /* Run scripts and remove from environment */
    if (player) {
        PlayerSAO *playersao = player->getPlayerSAO();
        assert(playersao);
        playersao->clearChildAttachments();
        playersao->clearParentAttachment();
        // inform connected clients FIXME: Pass into Lua or Julia environment
        /*const std::string &player_name = player->getName();
        NetworkPacket notice(TOCLIENT_UPDATE_PLAYER_LIST, 0, PEER_ID_INEXISTENT);
        // (u16) 1 + std::string represents a vector serialization representation
        notice << (u8) PLAYER_LIST_REMOVE  << (u16) 1 << player_name;
        m_clients.sendToAll(&notice);*/
        // run scripts
        //m_script->on_leaveplayer(playersao, CDR_LEAVE);
        (reinterpret_cast<void*(*)(PlayerSAO*, ClientDeletionReason)>(AddonsCallbacks[CALLBACK_ON_LEAVEPLAYER]))(playersao, CDR_LEAVE);
        playersao->disconnected(true);
    }
    actionstream << "a player leaves the server [Going to other server] [" << player->getName() << "]" << std::endl;
}

//BEGIN PEERID_MANAGEMENT

void Server::DenySudoAccess(session_t peer_id) {
    NetworkPacket pkt(TOCLIENT_DENY_SUDO_MODE, 0, peer_id);
    Send(&pkt);
}


void Server::DenyAccess(session_t peer_id, AccessDeniedCode reason, const std::string &custom_reason, bool reconnect) {
    SendAccessDenied(peer_id, reason, custom_reason, reconnect);
    m_clients.event(peer_id, CSE_SetDenied);
    DisconnectPeer(peer_id);
}

void Server::DisconnectPeer(session_t peer_id) {
    m_con->DisconnectPeer(peer_id);
}

//END PEERID_MANAGEMENT

void Server::DeletePlayer(u16 p) {
    DeleteClient(p, CDR_LEAVE);
}

//END RemoveAddPlayer

//BEGIN PlayerData


bool Server::getClientConInfo(session_t peer_id, con::rtt_stat_type type, float* retval) {
    *retval = m_con->getPeerStat(peer_id, type);
    return *retval != -1;
}

bool Server::getClientInfo(uint16_t peer_id, ClientInfo &ret)
{
    if (!ServersNetworkObject->AreSlave) {
        m_clients.lock();
        RemoteClient* client = m_clients.lockedGetClientNoEx(peer_id, CS_Invalid);
        if (!client) {
            m_clients.unlock();
            return false;
        }
        ret.state = client->getState();
        ret.addr = client->getAddress();
        ret.uptime = client->uptime();
        ret.ser_vers = client->serialization_version;
        ret.prot_vers = client->net_proto_version;
        ret.major = client->getMajor();
        ret.minor = client->getMinor();
        ret.patch = client->getPatch();
        ret.vers_string = client->getFullVer();
        ret.platform = client->getPlatform();
        ret.sysinfo = client->getSysInfo();
        ret.lang_code = client->getLangCode();
        m_clients.unlock();
    } else {
        if (!ClientDataTable.Has(peer_id)) {
            return false;
        }
        ClientDataHelper* client = ClientDataTable.Get(peer_id);
        ret.state = client->getState();
        ret.addr = client->getAddress();
        ret.uptime = client->uptime();
        ret.ser_vers = client->serialization_version;
        ret.prot_vers = client->net_proto_version;
        ret.major = client->getMajor();
        ret.minor = client->getMinor();
        ret.patch = client->getPatch();
        ret.vers_string = client->getFullVer();
        ret.platform = client->getPlatform();
        ret.sysinfo = client->getSysInfo();
        ret.lang_code = client->getLangCode();
    }

    return true;
}

uint8_t Server::getSerializationVersion(session_t peer_id) {
    RemoteClient *client = m_clients.lockedGetClientNoEx(peer_id, CS_Active);
    if (!client)
        return SER_FMT_VER_INVALID;
    return client->serialization_version;
}

#include "handles.h"
std::set<std::string> Server::getPlayerEffectivePrivs(const std::string &name)
{
    std::set<std::string> privs;
    getAuth(name, NULL, &privs, nullptr); 
    return privs;
}

bool Server::checkPriv(const std::string &name, const std::string &priv)
{
    std::set<std::string> privs = getPlayerEffectivePrivs(name);
    return (privs.count(priv) != 0);
}

//END PlayerData

void Server::notifyPlayers(const std::wstring &msg)
{
    SendChatMessage(PEER_ID_INEXISTENT, ChatMessage(msg));
}

//BEGIN GETTERS

ClientDataHelper *Server::getClientCDH(uint16_t p_id) {
    if (ClientDataTable.Has(p_id)) {
        return ClientDataTable.Get(p_id);
    }
}
RemoteClient *Server::getClient(uint16_t peer_id, ClientState state_min)
{
    RemoteClient *client = getClientNoEx(peer_id,state_min);

    return client;
}
RemoteClient *Server::getClientNoEx(uint16_t peer_id, ClientState state_min)
{
    return m_clients.getClientNoEx((session_t)peer_id, state_min);
}

std::string Server::getPlayerName(session_t peer_id)
{
    RemotePlayer *player = m_env->getPlayer(peer_id);
    if (!player)
        return "[id="+itos(peer_id)+"]";
    return player->getName();
}

PlayerSAO *Server::getPlayerSAO(session_t peer_id)
{
    RemotePlayer *player = m_env->getPlayer(peer_id);
    if (!player) {
        warningstream << "Possible crash: Unknown player: " << peer_id << std::endl;
        return NULL;
    }
    return player->getPlayerSAO();
}

//END GETTERS

//BEGIN AUTH

//Must be an network peer
void Server::acceptAuth(session_t peer_id, bool forSudoMode)
{
    if (!forSudoMode) {
        RemoteClient* client = getClient(peer_id, CS_Invalid);

        NetworkPacket resp_pkt(TOCLIENT_AUTH_ACCEPT, 1 + 6 + 8 + 4, peer_id, client->net_proto_version);

        // Right now, the auth mechs don't change between login and sudo mode.
        u32 sudo_auth_mechs = client->allowed_auth_mechs;
        client->allowed_sudo_mechs = sudo_auth_mechs;

        resp_pkt << v3f(0,0,0) << (u64) m_env->getServerMap().getSeed()
        << g_settings->getFloat("dedicated_server_step")
        << sudo_auth_mechs;

        Send(&resp_pkt);
        m_clients.event(peer_id, CSE_AuthAccept);
    } else {
        NetworkPacket resp_pkt(TOCLIENT_ACCEPT_SUDO_MODE, 1 + 6 + 8 + 4, peer_id);

        // We only support SRP right now
        u32 sudo_auth_mechs = AUTH_MECHANISM_FIRST_SRP;

        resp_pkt << sudo_auth_mechs;
        Send(&resp_pkt);
        m_clients.event(peer_id, CSE_SudoSuccess);
    }
}

//END AUTH

//BEGIN GamePlay

void Server::RespawnPlayer(uint16_t peer_id)
{
    PlayerSAO *playersao = getPlayerSAO(peer_id);
    assert(playersao);

    infostream << "Server::RespawnPlayer(): Player "
    << playersao->getPlayer()->getName()
    << " respawns" << std::endl;

    playersao->setHP(playersao->accessObjectProperties()->hp_max, PlayerHPChangeReason(PlayerHPChangeReason::RESPAWN));
    playersao->setBreath(playersao->accessObjectProperties()->breath_max);

    bool repositioned = (reinterpret_cast<bool*(*)(PlayerSAO*, ClientDeletionReason)>(AddonsCallbacks[CALLBACK_ON_LEAVEPLAYER]))(playersao, CDR_LEAVE);
    if (!repositioned) {
        // setPos will send the new position to client
        const v3f pos = findSpawnPos();
        actionstream << "Moving " << playersao->getPlayer()->getName() <<
        " to spawnpoint at (" << pos.X << ", " << pos.Y << ", " <<
        pos.Z << ")" << std::endl;
        playersao->setPos(pos);
    }
    SendPlayerHP(peer_id);
}


void Server::DiePlayer(uint16_t peer_id, const PlayerHPChangeReason &reason)
{
    PlayerSAO *playersao = getPlayerSAO(peer_id);
    assert(playersao);

    infostream << "Server::DiePlayer(): Player "
    << playersao->getPlayer()->getName()
    << " dies" << std::endl;

    playersao->setHP(0, reason);
    playersao->clearParentAttachment();

    // Trigger scripted stuff
    //m_script->on_dieplayer(playersao, reason);
    (reinterpret_cast<void*(*)(PlayerSAO*, PlayerHPChangeReason)>(AddonsCallbacks[CALLBACK_ON_LEAVEPLAYER]))(playersao, reason);

    SendPlayerHP(peer_id);
    SendDeathscreen(peer_id, false, v3f(0,0,0));
}

//END GamePlay

void Server::setPlayerEyeOffset(RemotePlayer *player, const v3f &first, const v3f &third)
{
    sanity_check(player);
    player->eye_offset_first = first;
    player->eye_offset_third = third;
    SendEyeOffset(player->getPeerId(), first, third);
}

void Server::setLocalPlayerAnimations(RemotePlayer *player, v2s32 animation_frames[4], f32 frame_speed) {
    sanity_check(player);
    player->setLocalAnimations(animation_frames, frame_speed);
    SendLocalPlayerAnimations(player->getPeerId(), animation_frames, frame_speed);
}

//BEGIN MapManager
//NOTE: This only hides the player from the eyes of other players!

//Make player join on another map, this is like dissapearing in one and appearing on other
bool Server::setPlayerOnMap(RemotePlayer *player, uint16_t mapid, v3f pos) {
    if (Maps.find(mapid) == Maps.end()) {
        errorstream << FUNCTION_NAME << ": Unknown map: " << mapid << std::endl;
        return false;
    }
    errorstream << FUNCTION_NAME << ": Player joins on map: " << mapid << std::endl;
    // Let's delete every object from player vision (Not the attached ones)
    SendRemoveObjectsToClient(player->player_id, true);

    // Store data about player pos, store last position and last mapid (FIXME: There should be last visited mapid)
    // MultithreadMap<uint16_t, RemotePlayer*> map_ = MapToPlayers.Get(mapid);
    PlayerDataOMM data = PlayerToMap.Get(player->getName());
    data.mapid = mapid;
    data.MAP[player->m_map_id] = player->getPlayerSAO()->getBasePosition();

    PlayerToMap.Set(player->getName(), data);

    player->m_map_id = mapid;

    SendMovePlayer(player->getPeerId(), pos); // [[BIG SHOT]]

    //There should be autojoin player soon, now, we will save the info
    //m_smfs->clock0.store(0);
    saveMapFiles();
    return true;
}

bool Server::unSetPlayerOnMap(RemotePlayer *player) {
    return false;
}

//END MapManager

std::list<RemotePlayer*> Server::getPlayersInMap(uint16_t mapid) {
    
}

/*
 Just for help:

 struct PlayerInternalInfo {
 uint16_t PlayerID;
 uint16_t ServerID;
 session_t PeerID;
 bool PlayingOnServ;
 };
 */









