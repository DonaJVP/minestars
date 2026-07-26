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
#include "../settings.h"
#include "../ServerNetworkEngine.h"
//#include "lib/mt_map.h"
#include "../server/player_sao.h"
#include "../server/unit_sao.h"
#include "../chat.h"
#include "../chat_interface.h"
#include "../player.h"
#include "../remoteplayer.h"
#include "../clientiface.h"
#include "../slave_helpers.h"
#include "../skyparams.h"
#include "../script/scripting_server.h"
#include "../slave_helpers.h"
#include "../network/serveropcodes.h"
#include "../nodemetadata.h"
#include "../inventorymanager.h"
#include "../inventory.h"
#include "../mapblock.h"
#include "../server/serverinventorymgr.h"
#include "../inventory.h"
#include "../addons/cblks_def.hpp"
#include "handles.h"
#include <iostream>
#include <cstdint>
#include <string>

std::string _F_helper(const std::string &name, const std::string &message) {
    return name+(std::string(": "))+message;
}

_formatChatMessage formatChatMessage = &_F_helper;

//ARGS: peer_id: p.id or session pid; protocol_version.
void Server::SendMovement(uint16_t peer_id, uint16_t protocol_version)
{
    std::ostringstream os(std::ios_base::binary);

    NetworkPacket pkt(TOCLIENT_MOVEMENT, 12 * sizeof(float), (session_t)peer_id, protocol_version, (uint8_t)ThisServID);

    pkt << g_settings->getFloat("movement_acceleration_default");
    pkt << g_settings->getFloat("movement_acceleration_air");
    pkt << g_settings->getFloat("movement_acceleration_fast");
    pkt << g_settings->getFloat("movement_speed_walk");
    pkt << g_settings->getFloat("movement_speed_crouch");
    pkt << g_settings->getFloat("movement_speed_fast");
    pkt << g_settings->getFloat("movement_speed_climb");
    pkt << g_settings->getFloat("movement_speed_jump");
    pkt << g_settings->getFloat("movement_liquid_fluidity");
    pkt << g_settings->getFloat("movement_liquid_fluidity_smooth");
    pkt << g_settings->getFloat("movement_liquid_sink");
    pkt << g_settings->getFloat("movement_gravity");

    Send(&pkt);
}

void Server::SendPlayerHPOrDie(PlayerSAO *playersao, const PlayerHPChangeReason &reason)
{
    if (playersao->isImmortal())
        return;
    uint16_t id = playersao->getPeerID(); //PeerID must equal to playerid IF a slave
    bool is_alive = !playersao->isDead();
    if (is_alive)
        SendPlayerHP(id);
    else
        DiePlayer(id, reason);
}

void Server::SendHP(uint16_t peer_id, uint16_t hp) {
    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket pkt(TOCLIENT_HP, 1, peer_id);
        // Minetest 0.4 uses 8-bit integers for HPP.
        if (m_clients.getProtocolVersion(peer_id) >= 37) {
            pkt << hp;
        } else {
            uint8_t raw_hp = hp & 0xFF;
            pkt << raw_hp;
        }
        Send(&pkt);
    } else {
        NetworkPacket pkt(TOCLIENT_HP, 5, peer_id, 0, ThisServID);
        pkt << hp;
        Send(&pkt);
    }
}

void Server::SendBreath(uint16_t peer_id, uint16_t breath) {
    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket pkt(TOCLIENT_BREATH, 2, peer_id);
        pkt << (uint16_t) breath;
        Send(&pkt);
    } else {
        NetworkPacket pkt(TOCLIENT_BREATH, 5, peer_id, 0, ThisServID);
        pkt << (uint16_t)breath;
        Send(&pkt);
    }
}

void Server::SendDeathscreen(uint16_t peer_id, bool set_camera_point_target, v3f camera_point_target) {
    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket pkt(TOCLIENT_DEATHSCREEN, 1 + sizeof(v3f), peer_id);
        pkt << set_camera_point_target << camera_point_target;
        Send(&pkt);
    } else {
        NetworkPacket pkt(TOCLIENT_DEATHSCREEN, 5, peer_id, 0, ThisServID);
        pkt <<set_camera_point_target<<camera_point_target;
        Send(&pkt);
    }
}

//BEGIN NON_STATIC_FUNCTIONS

void Server::SendChatMessage(session_t peer_id, const ChatMessage &message)
{
    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket legacypkt(TOCLIENT_CHAT_MESSAGE_OLD, 0, peer_id);
        NetworkPacket pkt(TOCLIENT_CHAT_MESSAGE, 0, peer_id);
        legacypkt << message.message;
        u8 version = 1;
        u8 type = message.type;
        pkt << version << type << message.sender << message.message << static_cast<u64>(message.timestamp);
        if (peer_id != PEER_ID_INEXISTENT) {
            RemotePlayer *player = m_env->getPlayer(peer_id);
            if (!player)
                return;
            if (player->protocol_version < 35)
                Send(&legacypkt);
            else
                Send(&pkt);
        } else {
            const RemoteClientMap &clients = m_clients.getClientList();
            // Send data to every connected client, except those that are playing on other serv
            m_clients.lock();
            for (const auto &client_it : clients) {
                RemoteClient *client = client_it.second;
                RemotePlayer *player = m_env->getPlayer(client->peer_id);
                if (!player)
                    continue;
                if (player->OnServer->IsApplied)
                    continue;
                if (client->net_proto_version >= 35) {
                    pkt.setPeerID(client->peer_id);
                    Send(&pkt);
                } else if (client->net_proto_version != 0) {
                    legacypkt.setPeerID(client->peer_id);
                    Send(&pkt);
                }
            }
            m_clients.unlock();
        }
    } else {
        NetworkPacket pkt(TOCLIENT_CHAT_MESSAGE, 5, peer_id, 0, ThisServID);
        pkt << (uint8_t)1;
        pkt << message.type;
        pkt << message.sender << message.message << static_cast<uint64_t>(message.timestamp);
        Send(&pkt);
    }
}

void Server::SendShowFormspecMessage(uint16_t peer_id, const std::string &formspec, const std::string &formname) {

    NetworkPacket pkt;

    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket _pkt(TOCLIENT_SHOW_FORMSPEC, 0, peer_id);
        pkt = _pkt;
    } else {
        NetworkPacket _pkt(TOCLIENT_SHOW_FORMSPEC, 0, peer_id, 0, ThisServID);
        pkt = _pkt;
    }

    if (formspec.empty()){
        //the client should close the formspec
        //but make sure there wasn't another one open in meantime
        const auto it = m_formspec_state_data.find(peer_id);
        if (it != m_formspec_state_data.end() && it->second == formname) {
            m_formspec_state_data.erase(peer_id);
        }
        pkt.putLongString("");
    } else {
        m_formspec_state_data[peer_id] = formname;
        RemotePlayer *player = m_env->getPlayer(peer_id);
        if (player && player->protocol_version < 37)
            pkt.putLongString(insert_formspec_prepend(formspec, player->formspec_prepend));
        else
            pkt.putLongString(formspec);
    }

    pkt << formname;
    Send(&pkt);
}

//END NON_STATIC_FUNCTIONS

//BEGIN INVENTORYFUNC

bool Server::showFormspec(const char *playername, const std::string &formspec, const std::string &formname) {
    // m_env will be NULL if the server is initializing
    if (!m_env)
        return false;

    RemotePlayer *player = m_env->getPlayer(playername);
    if (!player)
        return false;

    SendShowFormspecMessage(player->getPeerId(), formspec, formname);
    return true;
}

void Server::SendInventory(PlayerSAO *sao, bool incremental)
{
    if (ServersNetworkObject->AreSlave) {
        RemotePlayer *player = sao->getPlayer();
        incremental &= player->protocol_version >= 38;
        UpdateCrafting(player);
        //Serialize it
        u16 SrvID = ThisServID;
        NetworkPacket pkt(TOCLIENT_INVENTORY, 3, sao->getPlayerID(), 0, SrvID);
        std::ostringstream os(std::ios::binary);
        sao->getInventory()->serialize(os, incremental);
        sao->getInventory()->setModified(false);
        player->setModified(true);
        const std::string &s = os.str();
        pkt.putRawString(s.c_str(), s.size());
        Send(&pkt);
    } else {
        RemotePlayer *player = sao->getPlayer();
        incremental &= player->protocol_version >= 38;
        UpdateCrafting(player);
        NetworkPacket pkt(TOCLIENT_INVENTORY, 0, sao->getPeerID());
        std::ostringstream os(std::ios::binary);
        sao->getInventory()->serialize(os, incremental);
        sao->getInventory()->setModified(false);
        player->setModified(true);
        const std::string &s = os.str();
        pkt.putRawString(s.c_str(), s.size());
        Send(&pkt);
    }
}

void Server::sendDetachedInventory(Inventory *inventory, const std::string &name, uint16_t peer_id)
{
    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket pkt(TOCLIENT_DETACHED_INVENTORY, 0, peer_id);
        NetworkPacket legacy_pkt(TOCLIENT_DETACHED_INVENTORY, 0, peer_id);
        pkt << name;
        legacy_pkt << name;

        if (!inventory) {
            pkt << false; // Remove inventory
        } else {
            pkt << true; // Update inventory

            // Serialization & NetworkPacket isn't a love story
            std::ostringstream os(std::ios_base::binary);
            inventory->serialize(os);
            inventory->setModified(false);

            const std::string &os_str = os.str();
            pkt << static_cast<u16>(os_str.size()); // HACK: to keep compatibility with 5.0.0 clients
            pkt.putRawString(os_str);
            legacy_pkt.putRawString(os_str);
        }

        if (peer_id == PEER_ID_INEXISTENT) {
            m_clients.newSendToAll(&pkt);
            if (inventory)
                m_clients.oldSendToAll(&legacy_pkt);
        } else {
            RemoteClient *client = getClientNoEx(peer_id, CS_Created);
            if (!client) {
                warningstream << "Could not get client in sendDetachedInventory!"
                << std::endl;
            }

            if (!client || client->net_proto_version >= 37)
                Send(&pkt);
            else if (inventory)
                Send(&legacy_pkt);
        }
    } else {
        NetworkPacket pkt(TOCLIENT_DETACHED_INVENTORY, 0, peer_id, 0, (uint8_t)ThisServID);
        pkt << name;
        if (!inventory) {
            pkt << false; // Remove inventory
        } else {
            pkt << true; // Update inventory
            // Serialization & NetworkPacket isn't a love story
            std::ostringstream os(std::ios_base::binary);
            inventory->serialize(os);
            inventory->setModified(false);
            const std::string &os_str = os.str();
            pkt << static_cast<u16>(os_str.size()); // HACK: to keep compatibility with 5.0.0 clients
            pkt.putRawString(os_str);

        }
        Send(&pkt);
    }
}

//END INVENTORYFUNC

//BEGIN Particles

void Server::SendSpawnParticle(uint16_t peer_id, u16 protocol_version, const ParticleParameters &p) {
    static thread_local const float radius =
    g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE * BS;

    if (peer_id == PEER_ID_INEXISTENT) {
        if (!ServersNetworkObject->AreSlave) {
            std::vector<session_t> clients = m_clients.getClientIDs();
            const v3f pos = p.pos * BS;
            const float radius_sq = radius * radius;

            for (const session_t client_id : clients) {
                RemotePlayer *player = m_env->getPlayer(client_id);
                if (!player)
                    continue;

                PlayerSAO *sao = player->getPlayerSAO();
                if (!sao)
                    continue;

                // Do not send to distant clients
                if (sao->getBasePosition().getDistanceFromSQ(pos) > radius_sq)
                    continue;

                SendSpawnParticle(client_id, player->protocol_version, p);
            }
            return;
        } else {
            std::vector<RemotePlayer *> clients = m_env->getPlayers();
            const v3f pos = p.pos * BS;
            const float radius_sq = radius * radius;
            for (RemotePlayer *p_ : clients) {
                PlayerSAO *sao = p_->getPlayerSAO();
                if (!sao)
                    continue;
                // Do not send to distant clients
                if (sao->getBasePosition().getDistanceFromSQ(pos) > radius_sq)
                    continue;

                SendSpawnParticle(p_->player_id, p_->protocol_version, p);
            }
            return;
        }
    }
    assert(protocol_version != 0);

    NetworkPacket pkt(TOCLIENT_SPAWN_PARTICLE, 0, peer_id, protocol_version);

    if (ServersNetworkObject->AreSlave) {
        NetworkPacket _(TOCLIENT_SPAWN_PARTICLE, 0, peer_id, protocol_version, ThisServID);
        pkt = _;
    }

    {
        // NetworkPacket and iostreams are incompatible... OK
        std::ostringstream oss(std::ios_base::binary);
        p.serialize(oss, protocol_version);
        pkt.putRawString(oss.str());
    }

    Send(&pkt);
}

void Server::SendAddParticleSpawner(uint16_t peer_id, u16 protocol_version, const ParticleSpawnerParameters &p, u16 attached_id, u32 id) {
    static thread_local const float radius = g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE * BS;

    if (peer_id == PEER_ID_INEXISTENT) {
        if (!ServersNetworkObject->AreSlave) {
            std::vector<session_t> clients = m_clients.getClientIDs();
            const v3f pos = (p.minpos + p.maxpos) / 2.0f * BS;
            const float radius_sq = radius * radius;
            /* Don't send short-lived spawners to distant players.
            * This could be replaced with proper tracking at some point. */
            const bool distance_check = !attached_id && p.time <= 1.0f;

            for (const session_t client_id : clients) {
                RemotePlayer *player = m_env->getPlayer(client_id);
                if (!player)
                    continue;
                if (distance_check) {
                    PlayerSAO *sao = player->getPlayerSAO();
                    if (!sao)
                        continue;
                    if (sao->getBasePosition().getDistanceFromSQ(pos) > radius_sq)
                        continue;
                }
                SendAddParticleSpawner(client_id, player->protocol_version, p, attached_id, id);
            }
            return;
        } else {
            std::vector<RemotePlayer *> clients = m_env->getPlayers();
            const v3f pos = (p.minpos + p.maxpos) / 2.0f * BS;
            const float radius_sq = radius * radius;
            /* Don't send short-lived spawners to distant players.
             * This could be replaced with proper tracking at some point. */
            const bool distance_check = !attached_id && p.time <= 1.0f;

            for (RemotePlayer *p_ : clients) {
                if (!p_)
                    continue;
                if (distance_check) {
                    PlayerSAO *sao = p_->getPlayerSAO();
                    if (!sao)
                        continue;
                    if (sao->getBasePosition().getDistanceFromSQ(pos) > radius_sq)
                        continue;
                }
                SendAddParticleSpawner(p_->getPlayerID(), p_->protocol_version, p, attached_id, id);
            }
            return;
        }
    }
    assert(protocol_version != 0);

    //if (AreSlave && peer_id == PEER_ID_INEXISTENT) {
    //    peer_id = 0;
    //}

    NetworkPacket pkt(TOCLIENT_ADD_PARTICLESPAWNER, 100, peer_id, protocol_version);

    if (ServersNetworkObject->AreSlave) {
        NetworkPacket _(TOCLIENT_ADD_PARTICLESPAWNER, 0, peer_id, protocol_version, ThisServID);
        pkt = _;
    }

    pkt << p.amount << p.time << p.minpos << p.maxpos << p.minvel << p.maxvel << p.minacc << p.maxacc << p.minexptime << p.maxexptime << p.minsize << p.maxsize << p.collisiondetection;

    pkt.putLongString(p.texture);

    pkt << id << p.vertical << p.collision_removal << attached_id;
    {
        std::ostringstream os(std::ios_base::binary);
        p.animation.serialize(os, protocol_version);
        pkt.putRawString(os.str());
    }
    pkt << p.glow << p.object_collision;
    pkt << p.node.param0 << p.node.param2 << p.node_tile;

    Send(&pkt);
}

void Server::SendDeleteParticleSpawner(uint16_t peer_id, u32 id)
{
    NetworkPacket pkt(TOCLIENT_DELETE_PARTICLESPAWNER, 4, peer_id);

    if (ServersNetworkObject->AreSlave) {
        NetworkPacket pktn(TOCLIENT_DELETE_PARTICLESPAWNER, 0, peer_id, 0, ThisServID);
        pkt = pktn;
    }

    pkt << id;

    if (!ServersNetworkObject->AreSlave) {
        if (peer_id != PEER_ID_INEXISTENT)
            Send(&pkt);
        else
            m_clients.sendToAll(&pkt);
    } else {
        Send(&pkt);
    }

}

//END Particles

//BEGIN BLOCKS

void Server::SendBlocks(float dtime)
{
    MutexAutoLock envlock(m_env_mutex);
    //TODO check if one big lock could be faster then multiple small ones
    if (!ServersNetworkObject->AreSlave) {
        std::vector<PrioritySortedBlockTransfer> queue;
        u32 total_sending = 0;
        {
            std::vector<session_t> clients = m_clients.getClientIDs();
            m_clients.lock();
            for (const session_t client_id : clients) {
                RemoteClient *client = m_clients.lockedGetClientNoEx(client_id, CS_Active);
                if (!client)
                    continue;
                total_sending += client->getSendingCount();
                client->GetNextBlocks(m_env, Maps.at(PlayerToMap.Get(client->getName()).mapid)->m_emerge, dtime, queue);
            }
            m_clients.unlock();
        }
        // Sort.
        // Lowest priority number comes first.
        // Lowest is most important.
        std::sort(queue.begin(), queue.end());
        m_clients.lock();
        // Maximal total count calculation
        // The per-client block sends is halved with the maximal online users
        u32 max_blocks_to_send = (m_env->getPlayerCount() + g_settings->getU32("max_users"))
* g_settings->getU32("max_simultaneous_block_sends_per_client") / 4 + 1;
        //Map &map = m_env->getMap(); //headache

        for (const PrioritySortedBlockTransfer &block_to_send : queue) {
            if (total_sending >= max_blocks_to_send)
                break;
            
            try {
                (SessionToPlayer.at(block_to_send.peer_id));
            } catch (std::out_of_range &e) {
                continue;
            }
            
            // Fuck it.
            MapBlock *block = (Maps.at(PlayerToMap.Get(getPlayerName(SessionToPlayer[block_to_send.peer_id])).mapid)->m_map)->getBlockNoCreateNoEx(block_to_send.pos);
            if (!block)
                continue;

            RemoteClient *client = m_clients.lockedGetClientNoEx(block_to_send.peer_id, CS_Active);
            if (!client)
                continue;

            SendBlockNoLock(block_to_send.peer_id, block, client->serialization_version, client->net_proto_version);

            client->SentBlock(block_to_send.pos);
            total_sending++;
        }
        m_clients.unlock();
    } else { //is slave
        std::vector<PrioritySortedBlockTransferPID> queue;
        u32 total_sending = 0;
        {
            //Collect blocks to send
            ClientDataTable.Lock();
            std::unordered_map<uint16_t, ClientDataHelper*> *map = ClientDataTable.GetRawMap();
            for (auto it = map->begin(); it != map->end(); ++it) {
                ClientDataHelper *client = it->second;
                if (!client)
                    continue;
                total_sending += client->getSendingCount();
                client->GetNextBlocks(m_env, Maps.at(PlayerToMap.Get(client->getName()).mapid)->m_emerge, dtime,
queue);
            }
            ClientDataTable.unLock();
        }
        std::sort(queue.begin(), queue.end());
        u32 max_blocks_to_send = (m_env->getPlayerCount() + g_settings->getU32("max_users")) *
g_settings->getU32("max_simultaneous_block_sends_per_client") / 4 + 1;
        //send
        for (const PrioritySortedBlockTransferPID &block_to_send : queue) {
            if (total_sending >= max_blocks_to_send)
                break;
            MapBlock *block =
(Maps.at(PlayerToMap.Get(m_env->getPlayer(block_to_send.playerid)->getName()).mapid)->m_map)->getBlockNoCreateNoEx(block_to_send.pos);
            if (!block)
                continue;

            if (!ClientDataTable.Has(block_to_send.playerid))
                continue;

            ClientDataHelper *client = ClientDataTable.Get(block_to_send.playerid);

            SendBlockNoLock(block_to_send.playerid, block, client->serialization_version, client->net_proto_version);
            client->SentBlock(block_to_send.pos);
            total_sending++;
        }
    }
}

void Server::SendBlockNoLock(uint16_t peer_id, MapBlock *block, u8 ver, u16 net_proto_version)
{
    /*
     *	Create a packet with the block in the right format
     */
    thread_local const int net_compression_level = rangelim(g_settings->getS16("map_compression_level_net"), -1, 9);
    std::ostringstream os(std::ios_base::binary);

    RemotePlayer *player = m_env->getPlayer(peer_id);
    if (player && player->protocol_version < 37)
        block->serialize(os, ver, false, net_compression_level,
                         player->formspec_prepend);
        else
            block->serialize(os, ver, false, net_compression_level);
    block->serializeNetworkSpecific(os);
    std::string s = os.str();

    NetworkPacket pkt(TOCLIENT_BLOCKDATA, 2 + 2 + 2 + s.size(), peer_id);

    if (ServersNetworkObject->AreSlave) {
        NetworkPacket pktn(TOCLIENT_BLOCKDATA, 0, peer_id, 0, ThisServID);
        pkt = pktn;
    }

    pkt << block->getPos();
    pkt.putRawString(s.c_str(), s.size());
    Send(&pkt);
}

bool Server::SendBlock(uint16_t peer_id, const v3s16 &blockpos) {
    MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
    if (!block)
        return false;

    if (!ServersNetworkObject->AreSlave) {
        m_clients.lock();
        RemoteClient *client = m_clients.lockedGetClientNoEx(peer_id, CS_Active);
        if (!client || client->isBlockSent(blockpos)) {
            m_clients.unlock();
            return false;
        }
        SendBlockNoLock(peer_id, block, client->serialization_version,
                        client->net_proto_version);
        m_clients.unlock();
    } else {
        if (ClientDataTable.Has(peer_id))
            return false;

        ClientDataHelper *client = ClientDataTable.Get(peer_id);
        if (!client || client->isBlockSent(blockpos)) {
            return false;
        }
        SendBlockNoLock(peer_id, block, client->serialization_version, client->net_proto_version);
    }
    return true;
}

void Server::sendMetadataChanged(const std::list<v3s16> &meta_updates, float far_d_nodes)
{
    float maxd = far_d_nodes * BS;
    NodeMetadataList meta_updates_list(false);

    if (!ServersNetworkObject->AreSlave) {
        std::vector<session_t> clients = m_clients.getClientIDs();
        m_clients.lock();

        for (session_t i : clients) {
            RemoteClient *client = m_clients.lockedGetClientNoEx(i);
            if (!client)
                continue;

            if (client->net_proto_version < 37) {
                for (const v3s16 &pos : meta_updates) {
                    client->SetBlockNotSent(getNodeBlockPos(pos));
                }
                continue;
            }

            ServerActiveObject *player = m_env->getActiveObject(i);
            v3f player_pos = player ? player->getBasePosition() : v3f();

            for (const v3s16 &pos : meta_updates) {
                NodeMetadata *meta = m_env->getMap().getNodeMetadata(pos);

                if (!meta)
                    continue;

                v3s16 block_pos = getNodeBlockPos(pos);
                if (!client->isBlockSent(block_pos) || (player &&
                    player_pos.getDistanceFrom(intToFloat(pos, BS)) > maxd)) {
                    client->SetBlockNotSent(block_pos);
                continue;
                    }

                    // Add the change to send list
                    meta_updates_list.set(pos, meta);
            }
            if (meta_updates_list.size() == 0)
                continue;

            // Send the meta changes
            std::ostringstream os(std::ios::binary);
            meta_updates_list.serialize(os, client->net_proto_version, false, true);
            std::ostringstream oss(std::ios::binary);
            compressZlib(os.str(), oss);

            NetworkPacket pkt(TOCLIENT_NODEMETA_CHANGED, 0);
            pkt.putLongString(oss.str());
            m_clients.send(i, 0, &pkt, true);

            meta_updates_list.clear();
        }

        m_clients.unlock();
    } else {
        ClientDataTable.Lock();
        for (auto it = ClientDataTable.GetRawMap()->begin(); it != ClientDataTable.GetRawMap()->end(); ++it) {
            if (!ClientDataTable.Has(it->first))
                continue;
            ClientDataHelper *client = ClientDataTable.Get(it->first);
            RemotePlayer *p = m_env->getPlayer(it->first);

            if (client->net_proto_version < 37) {
                for (const v3s16 &pos : meta_updates) {
                    client->SetBlockNotSent(getNodeBlockPos(pos));
                }
                continue;
            }

            ServerActiveObject *player = m_env->getActiveObject(p->IdForSao);
            v3f player_pos = player ? player->getBasePosition() : v3f();

            for (const v3s16 &pos : meta_updates) {
                NodeMetadata *meta = m_env->getMap().getNodeMetadata(pos);

                if (!meta)
                    continue;

                v3s16 block_pos = getNodeBlockPos(pos);
                if (!client->isBlockSent(block_pos) || (player && player_pos.getDistanceFrom(intToFloat(pos, BS)) > maxd)) {
                    client->SetBlockNotSent(block_pos);
                    continue;
                }

                // Add the change to send list
                meta_updates_list.set(pos, meta);
            }
            if (meta_updates_list.size() == 0)
                continue;

            // Send the meta changes
            std::ostringstream os(std::ios::binary);
            meta_updates_list.serialize(os, client->net_proto_version, false, true);
            std::ostringstream oss(std::ios::binary);
            compressZlib(os.str(), oss);

            NetworkPacket pkt(TOCLIENT_NODEMETA_CHANGED, 0);



            pkt.putLongString(oss.str());
            //m_clients.send(i, 0, &pkt, true);
            Send(&pkt);

            meta_updates_list.clear();
        }
        ClientDataTable.unLock();
    }
}

void Server::sendAddNode(v3s16 p, MapNode n, std::unordered_set<u16> *far_players, float far_d_nodes, bool remove_metadata)
{
    float maxd = far_d_nodes * BS;
    v3f p_f = intToFloat(p, BS);
    v3s16 block_pos = getNodeBlockPos(p);

    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket pkt(TOCLIENT_ADDNODE, 6 + 2 + 1 + 1 + 1);

        pkt << p << n.param0 << n.param1 << n.param2 << (u8) (remove_metadata ? 0 : 1);

        std::vector<session_t> clients = m_clients.getClientIDs();
        m_clients.lock();

        for (session_t client_id : clients) {
            RemoteClient *client = m_clients.lockedGetClientNoEx(client_id);
            if (!client)
                continue;

            RemotePlayer *player = m_env->getPlayer(client_id);

            if (!player) {
                //Table broken, recall function
                sendAddNode(p, n, far_players, far_d_nodes, remove_metadata);
                return;
            }


            if (player->OnServer->IsApplied)
                continue;

            PlayerSAO *sao = player ? player->getPlayerSAO() : nullptr;

            // If player is far away, only set modified blocks not sent
            if (!client->isBlockSent(block_pos) || (sao &&
                sao->getBasePosition().getDistanceFrom(p_f) > maxd)) {
                if (far_players)
                    far_players->emplace(client_id);
                else
                    client->SetBlockNotSent(block_pos);
                continue;
                }

                // Send as reliable
                m_clients.send(client_id, 0, &pkt, true);
        }

        m_clients.unlock();
    } else {
        NetworkPacket pkt(0x83, 6 + 2 + 2 + 1 + 1 + 1 + 1, PEER_ID_INEXISTENT, 0, (uint8_t)ThisServID);

        u8 SrvID = (u8)ThisServID;

        pkt << SrvID;

        std::vector<u16> playersid_tosend;

        //make like a simple table just to send 1 packet with all players data to don't send 20+ packets to 1 server or it will be overloaded
        // [0: u16] PLAYERS COUNT [0-INF] PLAYERS [INF-**] DATA

        //pkt << p << n.param0 << n.param1 << n.param2 << (u8) (remove_metadata ? 0 : 1);
        ClientDataTable.Lock();
        for (auto it = ClientDataTable.GetRawMap()->begin(); it != ClientDataTable.GetRawMap()->end(); ++it) {
            ClientDataHelper *client = it->second;
            RemotePlayer *player = m_env->getPlayer(it->first);
            PlayerSAO *sao = player ? player->getPlayerSAO() : nullptr;
            if (!client->isBlockSent(block_pos) || (sao && sao->getBasePosition().getDistanceFrom(p_f) > maxd)) {

                if (far_players)
                    far_players->emplace(client->GetPlayerID_());
                else
                    client->SetBlockNotSent(block_pos);

                continue;
            }

            //pkt << SrvID << it->first << p << n.param0 << n.param1 << n.param2 << (u8) (remove_metadata ? 0 : 1);
            playersid_tosend.push_back(it->first);
        }
        ClientDataTable.unLock();
        //iterate
        u16 playercount = (u16)playersid_tosend.size();
        pkt << playercount;
        for (u16 pid : playersid_tosend) {
            pkt << pid;
        }
        pkt << p << n.param0 << n.param1 << n.param2 << (u8) (remove_metadata ? 0 : 1);
        Send(&pkt);
    }
}

void Server::sendRemoveNode(v3s16 p, std::unordered_set<u16> *far_players,
                            float far_d_nodes)
{
    float maxd = far_d_nodes * BS;
    v3f p_f = intToFloat(p, BS);
    v3s16 block_pos = getNodeBlockPos(p);
    if (!ServersNetworkObject->AreSlave) {
        NetworkPacket pkt(TOCLIENT_REMOVENODE, 6);
        pkt << p;

        std::vector<session_t> clients = m_clients.getClientIDs();
        m_clients.lock();

        for (session_t client_id : clients) {
            RemoteClient *client = m_clients.lockedGetClientNoEx(client_id);
            if (!client)
                continue;

            RemotePlayer *player = m_env->getPlayer(client_id);
            if (player->OnServer->IsApplied)
                continue;
            PlayerSAO *sao = player ? player->getPlayerSAO() : nullptr;

            // If player is far away, only set modified blocks not sent
            if (!client->isBlockSent(block_pos) || (sao &&
                sao->getBasePosition().getDistanceFrom(p_f) > maxd)) {
                if (far_players)
                    far_players->emplace(client_id);
                else
                    client->SetBlockNotSent(block_pos);
                continue;
                }

                // Send as reliable
                m_clients.send(client_id, 0, &pkt, true);
        }

        m_clients.unlock();
    } else {
        NetworkPacket pkt(0x67, 0, PEER_ID_INEXISTENT, 0, ThisServID);
        pkt << p;
        std::vector<u16> playersid_tosend;
        ClientDataTable.Lock();
        for (auto it = ClientDataTable.GetRawMap()->begin(); it != ClientDataTable.GetRawMap()->end(); ++it) {
            RemotePlayer *player = m_env->getPlayer(it->second->GetPlayerID_());
            PlayerSAO *sao = player ? player->getPlayerSAO() : nullptr;
            // If player is far away, only set modified blocks not sent
            if (!it->second->isBlockSent(block_pos) || (sao &&
                sao->getBasePosition().getDistanceFrom(p_f) > maxd)) {
                if (far_players)
                    far_players->emplace(it->first);
                else
                    it->second->SetBlockNotSent(block_pos);
                continue;
                }
                playersid_tosend.push_back(it->first);
        }
        ClientDataTable.unLock();

        u16 size_ = (u16)playersid_tosend.size();
        pkt << size_;
        for (u16 p : playersid_tosend) {
            pkt << p;
        }
        Send(&pkt);
    }
}

//END BLOCKS

//Must don't be used in slave serv as it are tool for proxy
void Server::SendCSMRestrictionFlags(session_t peer_id)
{
    const u16 protocol_version = m_clients.getProtocolVersion(peer_id);
    if (protocol_version < 35 && protocol_version != 0)
        return;

    NetworkPacket pkt(TOCLIENT_CSM_RESTRICTION_FLAGS,
                      sizeof(m_csm_restriction_flags) + sizeof(m_csm_restriction_noderange), peer_id);
    pkt << m_csm_restriction_flags << m_csm_restriction_noderange;
    Send(&pkt);
}

//BEGIN Huds

//m_clients.getProtocolVersion(peer_id)
void Server::SendHUDAdd(uint16_t peer_id, u32 id, HudElement *form) {
    NetworkPacket pkt(TOCLIENT_HUDADD, 0 , peer_id, 0);

    if (ServersNetworkObject->AreSlave) {
        uint16_t proto = ClientDataTable.Get(peer_id)->net_proto_version;
        NetworkPacket pktt(TOCLIENT_HUDADD, 0, peer_id, proto, ThisServID);
        pkt = pktt;
    }

    pkt << id << (u8) form->type << form->pos << form->name << form->scale
    << form->text << form->number << form->item << form->dir
    << form->align << form->offset << form->world_pos << form->size
    << form->z_index << form->text2;

    Send(&pkt);
}

void Server::SendHUDRemove(uint16_t peer_id, u32 id) {
    NetworkPacket pkt(TOCLIENT_HUDRM, 4, peer_id);
    if (ServersNetworkObject->AreSlave) {
        uint16_t proto = ClientDataTable.Get(peer_id)->net_proto_version;
        NetworkPacket pktt(TOCLIENT_HUDRM, 0, peer_id, proto, ThisServID);
        pkt = pktt;
    }
    pkt << id;
    Send(&pkt);
}

void Server::SendHUDChange(uint16_t peer_id, u32 id, HudElementStat stat, void *value)
{
    NetworkPacket pkt(TOCLIENT_HUDCHANGE, 0, peer_id);

    if (ServersNetworkObject->AreSlave) {
        uint16_t proto = ClientDataTable.Get(peer_id)->net_proto_version;
        NetworkPacket pktt(TOCLIENT_HUDCHANGE, 0, peer_id, proto, ThisServID);
        pkt = pktt;
    } else {
        NetworkPacket pkt2(TOCLIENT_HUDCHANGE, 0, peer_id, m_clients.getProtocolVersion(peer_id));
        pkt = pkt2;
    }

    pkt << id << (u8) stat;

    switch (stat) {
        case HUD_STAT_POS:
        case HUD_STAT_SCALE:
        case HUD_STAT_ALIGN:
        case HUD_STAT_OFFSET:
            pkt << *(v2f *) value;
            break;
        case HUD_STAT_NAME:
        case HUD_STAT_TEXT:
        case HUD_STAT_TEXT2:
            pkt << *(std::string *) value;
            break;
        case HUD_STAT_WORLD_POS:
            pkt << *(v3f *) value;
            break;
        case HUD_STAT_SIZE:
            pkt << *(v2s32 *) value;
            break;
        case HUD_STAT_NUMBER:
        case HUD_STAT_ITEM:
        case HUD_STAT_DIR:
        default:
            pkt << *(u32 *) value;
            break;
    }

    Send(&pkt);
}

void Server::SendHUDSetFlags(uint16_t peer_id, u32 flags, u32 mask)
{
    NetworkPacket pkt(TOCLIENT_HUD_SET_FLAGS, 4 + 4, peer_id);

    if (ServersNetworkObject->AreSlave) {
        NetworkPacket pktt(TOCLIENT_HUD_SET_FLAGS, 0, peer_id, 0, ThisServID);
        pkt = pktt;
    }

    flags &= ~(HUD_FLAG_HEALTHBAR_VISIBLE | HUD_FLAG_BREATHBAR_VISIBLE);

    pkt << flags << mask;

    Send(&pkt);
}

void Server::SendHUDSetParam(uint16_t peer_id, u16 param, const std::string &value) {
    NetworkPacket pkt(TOCLIENT_HUD_SET_PARAM, 0, peer_id);

    if (ServersNetworkObject->AreSlave) {
        NetworkPacket pktt(TOCLIENT_HUD_SET_PARAM, 0, peer_id, 0, ThisServID);
        pkt = pktt;
    }

    pkt << param << value;
    Send(&pkt);
}

//END Huds

//BEGIN Sky

void Server::SendSetSky(uint16_t peer_id, const SkyboxParams &params)
{
    NetworkPacket pkt(TOCLIENT_SET_SKY, 0, peer_id, 0, ThisServID);

    RemotePlayer *player = m_env->getPlayer(peer_id);

    uint16_t protover = ServersNetworkObject->AreSlave ? player->protocol_version : m_clients.getProtocolVersion(peer_id);

    // Handle prior clients here
    if (protover < 39) {
        pkt << params.bgcolor << params.type << (u16) params.textures.size();

        for (const std::string& texture : params.textures)
            pkt << texture;

        pkt << params.clouds;
    } else { // Handle current clients and future clients
        pkt << params.bgcolor << params.type
        << params.clouds << params.fog_sun_tint
        << params.fog_moon_tint << params.fog_tint_type;

        if (params.type == "skybox") {
            pkt << (u16) params.textures.size();
            for (const std::string &texture : params.textures)
                pkt << texture;
        } else if (params.type == "regular") {
            pkt << params.sky_color.day_sky << params.sky_color.day_horizon
            << params.sky_color.dawn_sky << params.sky_color.dawn_horizon
            << params.sky_color.night_sky << params.sky_color.night_horizon
            << params.sky_color.indoors;
        }
    }

    Send(&pkt);
}

void Server::SendSetSun(uint16_t peer_id, const SunParams &params) {
    NetworkPacket pkt(TOCLIENT_SET_SUN, 0, peer_id, 0, ThisServID);
    pkt << params.visible << params.texture
    << params.tonemap << params.sunrise
    << params.sunrise_visible << params.scale;
    Send(&pkt);
}

void Server::SendSetMoon(uint16_t peer_id, const MoonParams &params) {
    NetworkPacket pkt(TOCLIENT_SET_MOON, 0, peer_id, 0, ThisServID);
    pkt << params.visible << params.texture
    << params.tonemap << params.scale;
    Send(&pkt);
}

void Server::SendSetStars(session_t peer_id, const StarParams &params) {
    NetworkPacket pkt(TOCLIENT_SET_STARS, 0, peer_id, 0, ThisServID);
    pkt << params.visible << params.count
    << params.starcolor << params.scale;
    Send(&pkt);
}

void Server::SendCloudParams(uint16_t peer_id, const CloudParams &params)
{
    NetworkPacket pkt(TOCLIENT_CLOUD_PARAMS, 0, peer_id, (!ServersNetworkObject->AreSlave) ? m_clients.getProtocolVersion(peer_id) : m_env->getPlayer(peer_id)->protocol_version);
    pkt << params.density << params.color_bright << params.color_ambient
    << params.height << params.thickness << params.speed;
    Send(&pkt);
}

void Server::SendOverrideDayNightRatio(session_t peer_id, bool do_override, float ratio) {
    NetworkPacket pkt(TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO, 1 + 2, peer_id, 0, ThisServID);
    pkt << do_override << (u16) (ratio * 65535);
    Send(&pkt);
}


void Server::SendTimeOfDay(session_t peer_id, u16 time, f32 time_speed)
{
    if (!ServersNetworkObject->AreSlave) {
        //Only used to send time of day to a player
        if (peer_id != PEER_ID_INEXISTENT) {
            NetworkPacket pkt(TOCLIENT_TIME_OF_DAY, 0, peer_id,
                              m_clients.getProtocolVersion(peer_id));
            pkt << time << time_speed;

            Send(&pkt);
            return;
        }

        //this executes to update the world
        NetworkPacket pkt(TOCLIENT_TIME_OF_DAY, 0, peer_id, 37);
        NetworkPacket legacypkt(TOCLIENT_TIME_OF_DAY, 0, peer_id, 32);
        pkt << time << time_speed;
        legacypkt << time << time_speed; //HGZ
        //m_clients.sendToAllCompat(&pkt, &legacypkt, 37);
        m_clients.lock();
        const RemoteClientMap &clients = m_clients.getClientList();
        for (const auto &client_it : clients) {
            RemoteClient *client = client_it.second;

            RemotePlayer *player = m_env->getPlayer(client->peer_id);

            if (!player)
                continue;

            if (player->OnServer->IsApplied)
                continue;

            if (client->net_proto_version >= 37) {
                pkt.setPeerID(client->peer_id);
                Send(&pkt);
            } else if (client->net_proto_version != 0) {
                legacypkt.setPeerID(client->peer_id);
                Send(&pkt);
            }
        }
        m_clients.unlock();
    } else {
        //peer_id is PEER_ID_INEXISTENT in this case
        NetworkPacket pkt(TOCLIENT_TIME_OF_DAY, 0, 0, 37, (uint8_t)ThisServID);
        pkt << time << time_speed;
        Send(&pkt);
    }
}

//END Sky

//BEGIN PlayerDataV2

void Server::SendPlayerHP(uint16_t peer_id)
{
    PlayerSAO *playersao = getPlayerSAO(peer_id);
    assert(playersao);
    SendHP(peer_id, playersao->getHP());
    //m_script->player_event(playersao,"health_changed");
    (reinterpret_cast<void*(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_PLAYEREVENT]))(playersao, "health_changed");
    // Send to other clients
    playersao->sendPunchCommand();
}

void Server::SendPlayerBreath(PlayerSAO *sao)
{
	assert(sao);

    (reinterpret_cast<void*(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_PLAYEREVENT]))(sao, "breath_changed");
	//m_script->player_event(sao, "breath_changed");
    if (ServersNetworkObject->AreSlave)
        SendBreath(sao->getPlayerID(), sao->getBreath());
    else
        SendBreath(sao->getPeerID(), sao->getBreath());
}

void Server::SendMovePlayer(uint16_t peer_id, v3f overridepos)
{
    RemotePlayer *player = m_env->getPlayer(peer_id);
    assert(player);
    PlayerSAO *sao = player->getPlayerSAO();
    assert(sao);
    // Send attachment updates instantly to the client prior updating position
    sao->sendOutdatedData();
    NetworkPacket pkt(TOCLIENT_MOVE_PLAYER, sizeof(v3f) + sizeof(f32) * 2, peer_id, player->protocol_version, ThisServID);
    if (overridepos.X == 72000) //Only need one compare to see if it are defined. [Position can't exceed the 16bit value, but it are an 32bit value]
        pkt << sao->getBasePosition() << sao->getLookPitch() << sao->getRotation().Y;
    else
        pkt << overridepos << sao->getLookPitch() << sao->getRotation().Y;
    {
        v3f pos = (overridepos.X != 72000) ? overridepos : sao->getBasePosition();
        verbosestream << "Server: Sending TOCLIENT_MOVE_PLAYER"
        << " pos=(" << pos.X << "," << pos.Y << "," << pos.Z << ")"
        << " pitch=" << sao->getLookPitch()
        << " yaw=" << sao->getRotation().Y
        << std::endl;
    }
    Send(&pkt);
}

void Server::SendPlayerFov(uint16_t peer_id)
{
    NetworkPacket pkt(TOCLIENT_FOV, 4 + 1 + 4, peer_id, 0, ThisServID);
    PlayerFovSpec fov_spec = m_env->getPlayer(peer_id)->getFov();
    pkt << fov_spec.fov << fov_spec.is_multiplier << fov_spec.transition_time;
    Send(&pkt);
}

void Server::SendLocalPlayerAnimations(session_t peer_id, v2s32 animation_frames[4], f32 animation_speed)
{
    NetworkPacket pkt(TOCLIENT_LOCAL_PLAYER_ANIMATIONS, 0, peer_id, ServersNetworkObject->AreSlave ? ClientDataTable.Get(peer_id)->net_proto_version :
m_clients.getProtocolVersion(peer_id), ThisServID);

    pkt << animation_frames[0] << animation_frames[1] << animation_frames[2]
    << animation_frames[3] << animation_speed;

    Send(&pkt);
}


void Server::SendEyeOffset(session_t peer_id, v3f first, v3f third)
{
    NetworkPacket pkt(TOCLIENT_EYE_OFFSET, 0, peer_id, ServersNetworkObject->AreSlave ? ClientDataTable.Get(peer_id)->net_proto_version :
m_clients.getProtocolVersion(peer_id), ThisServID);
    pkt << first << third;
    Send(&pkt);
}


#include "handles.h"
void Server::SendPlayerPrivileges(uint16_t peer_id)
{
    RemotePlayer *player = m_env->getPlayer(peer_id);
    assert(player);
    std::set<std::string> privs;
    //m_script->getAuth(player->getName(), NULL, &privs);
    getAuth(player->getName(), nullptr, &privs, nullptr);
    NetworkPacket pkt(TOCLIENT_PRIVILEGES, 0, peer_id, 0, ThisServID);
    pkt << (u16) privs.size();

    for (const std::string &priv : privs) {
        pkt << priv;
        warningstream << priv << std::endl;
    }
    Send(&pkt);
}

void Server::SendPlayerSpeed(uint16_t peer_id, const v3f &added_vel)
{
    NetworkPacket pkt(TOCLIENT_PLAYER_SPEED, 0, peer_id, 0, ThisServID);
    pkt << added_vel;
    Send(&pkt);
}

//END PlayerDataV2

//BEGIN MINIMAP

void Server::SendMinimapModes(uint16_t peer_id, std::vector<MinimapMode> &modes, size_t wanted_mode)
{
    RemotePlayer *player = m_env->getPlayer(peer_id);
    assert(player);
    if (player->getPeerId() == PEER_ID_INEXISTENT)
        return;

    NetworkPacket pkt(TOCLIENT_MINIMAP_MODES, 0, peer_id, 0, ThisServID);
    pkt << (u16)modes.size() << (u16)wanted_mode;

    for (auto &mode : modes)
        pkt << (u16)mode.type << mode.label << mode.size << mode.texture << mode.scale;

    Send(&pkt);
}

//END MINIMAP

//BEGIN Inventory

void Server::sendDetachedInventories(uint16_t peer_id, bool incremental)
{
    // Lookup player name, to filter detached inventories just after
    std::string peer_name;
    if (peer_id != PEER_ID_INEXISTENT) {
        peer_name = (getClient(peer_id, CS_Created)->getName());
    }

    auto send_cb = [this, peer_id](const std::string &name, Inventory *inv) {
        sendDetachedInventory(inv, name, peer_id);
    };

    m_inventory_mgr->sendDetachedInventories(peer_name, incremental, send_cb);
}

void Server::SendPlayerInventoryFormspec(uint16_t peer_id)
{
    RemotePlayer *player = m_env->getPlayer(peer_id);
    assert(player);
    if (player->getPeerId() == PEER_ID_INEXISTENT)
        return;
    NetworkPacket pkt(TOCLIENT_INVENTORY_FORMSPEC, 0, peer_id, 0, ThisServID);
    if (player->protocol_version < 37)
        pkt.putLongString(insert_formspec_prepend(player->inventory_formspec, player->formspec_prepend));
    else
        pkt.putLongString(player->inventory_formspec);
    Send(&pkt);
}

void Server::SendPlayerFormspecPrepend(uint16_t peer_id)
{
    RemotePlayer *player = m_env->getPlayer(peer_id);
    assert(player);
    if (player->getPeerId() == PEER_ID_INEXISTENT)
        return;
    if (player->protocol_version < 37) {
        SendPlayerInventoryFormspec(peer_id);
        return;
    }

    NetworkPacket pkt(TOCLIENT_FORMSPEC_PREPEND, 0, peer_id, 0, ThisServID);
    pkt << player->formspec_prepend;
    Send(&pkt);
}

void Server::reportInventoryFormspecModified(const std::string &name)
{
    RemotePlayer *player = m_env->getPlayer(name.c_str());
    if (!player)
        return;
    SendPlayerInventoryFormspec(player->getPeerId());
}

void Server::reportFormspecPrependModified(const std::string &name)
{
    RemotePlayer *player = m_env->getPlayer(name.c_str());
    if (!player)
        return;
    SendPlayerFormspecPrepend(player->getPeerId());
}

//END Inventory

//BEGIN ObjectManagement


//Works only for proxy
void Server::SendActiveObjectRemoveAdd(RemoteClient *client, PlayerSAO *playersao)
{
    // Radius inside which objects are active
    static thread_local const s16 radius = g_settings->getS16("active_object_send_range_blocks") * MAP_BLOCKSIZE;
    // Radius inside which players are active
    static thread_local const bool is_transfer_limited = g_settings->exists("unlimited_player_transfer_distance") && !g_settings->getBool("unlimited_player_transfer_distance");
    static thread_local const s16 player_transfer_dist = g_settings->getS16("player_transfer_distance") * MAP_BLOCKSIZE;

    s16 player_radius = player_transfer_dist == 0 && is_transfer_limited ? radius : player_transfer_dist;

    s16 my_radius = MYMIN(radius, playersao->getWantedRange() * MAP_BLOCKSIZE);
    if (my_radius <= 0)
        my_radius = radius;

    std::queue<u16> removed_objects, added_objects;
    m_env->getRemovedActiveObjects(playersao, my_radius, player_radius, client->m_known_objects, removed_objects);
    m_env->getAddedActiveObjects(playersao, my_radius, player_radius, client->m_known_objects, added_objects);

    int removed_count = removed_objects.size();
    int added_count   = added_objects.size();

    if (removed_objects.empty() && added_objects.empty())
        return;

    char buf[4];
    std::string data;

    // Handle removed objects
    writeU16((u8*)buf, removed_objects.size());
    data.append(buf, 2);
    while (!removed_objects.empty()) {
        // Get object
        u16 id = removed_objects.front();
        ServerActiveObject* obj = m_env->getActiveObject(id);

        // Add to data buffer for sending
        writeU16((u8*)buf, id);
        data.append(buf, 2);
        // Remove from known objects
        client->m_known_objects.erase(id);

        if (obj && obj->m_known_by_count > 0)
            obj->m_known_by_count--;

        removed_objects.pop();
    }

    // Handle added objects
    writeU16((u8*)buf, added_objects.size());
    data.append(buf, 2);
    while (!added_objects.empty()) {
        // Get object
        u16 id = added_objects.front();
        ServerActiveObject *obj = m_env->getActiveObject(id);
        added_objects.pop();

        if (!obj) {
            warningstream << FUNCTION_NAME << ": NULL object id="
            << (int)id << std::endl;
            continue;
        }

        // Get object type
        u8 type = obj->getSendType();

        // Add to data buffer for sending
        writeU16((u8*)buf, id);
        data.append(buf, 2);
        writeU8((u8*)buf, type);
        data.append(buf, 1);

        data.append(serializeString32(
            obj->getClientInitializationData(client->net_proto_version)));

        // Add to known objects
        client->m_known_objects.insert(id);

        obj->m_known_by_count++;
    }

    //Debug
    //std::cout << std::string(data.c_str(), data.size()) << std::endl;

    NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD, data.size(), client->peer_id);
    pkt.putRawString(data.c_str(), data.size());
    Send(&pkt);

    verbosestream << "Server::SendActiveObjectRemoveAdd: "
    << removed_count << " removed, " << added_count << " added, "
    << "packet size is " << pkt.getSize() << std::endl;
}

void Server::SendRemoveObjectsToClient(u16 pid, bool keep_attached_ones) {
    if (!Players.Has(pid))
        return;
    RemotePlayer *p = m_env->getPlayer(Players.Get(pid).PeerID);
    if (!p) //lost frame
        return;
    RemoteClient *client = m_clients.lockedGetClientNoEx(p->getPeerId());
    PlayerSAO *sao = p->getPlayerSAO();
    u16 ID = sao->getId();
    char buf[4];
    std::string data;

    writeU16((u8*)buf, client->m_known_objects.size()-1); //-1 is player
    data.append(buf, 2);

    std::unordered_set<int> childs = sao->getAttachmentChildIds();

    for (std::set<u16>::iterator it = client->m_known_objects.begin(); it != client->m_known_objects.end(); ++it) {
        ServerActiveObject* obj = m_env->getActiveObject(*it);
        if (obj) {
            if (obj->m_known_by_count > 0)
                obj->m_known_by_count--;
            if (obj->getId() == ID) //skip player FIXME!
                continue;
        }

        if (keep_attached_ones && childs.find(obj->getId()) != childs.end())
            continue; //Childs should not be deleted in player vision if changing of map

        writeU16((u8*)buf, *it);
        data.append(buf, 2);
    }
    client->m_known_objects.erase(client->m_known_objects.begin(), client->m_known_objects.end());
    //prepare data
    NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD, data.size(), client->peer_id);
    pkt.putRawString(data.c_str(), data.size());
    Send(&pkt);
}

void Server::SendActiveObjectRemoveAdd_SLAVE(ClientDataHelper *client, PlayerSAO *playersao)
{
    ///if (client->FirstStep) {
    //	client->FirstStep = false;
    //	return;
    //}
    // Radius inside which objects are active
    static thread_local const s16 radius = g_settings->getS16("active_object_send_range_blocks") * MAP_BLOCKSIZE;
    // Radius inside which players are active
    static thread_local const bool is_transfer_limited = g_settings->exists("unlimited_player_transfer_distance") && !g_settings->getBool("unlimited_player_transfer_distance");
    static thread_local const s16 player_transfer_dist = g_settings->getS16("player_transfer_distance") * MAP_BLOCKSIZE;

    s16 player_radius = player_transfer_dist == 0 && is_transfer_limited ? radius : player_transfer_dist;

    s16 my_radius = MYMIN(radius, playersao->getWantedRange() * MAP_BLOCKSIZE);
    if (my_radius <= 0)
        my_radius = radius;

    std::queue<u16> removed_objects, added_objects, ro, ao;
    m_env->getRemovedActiveObjects(playersao, my_radius, player_radius, client->m_known_objects, removed_objects);
    m_env->getAddedActiveObjects(playersao, my_radius, player_radius, client->m_known_objects, added_objects);

    int removed_count = removed_objects.size();
    int added_count = added_objects.size();

    if (removed_objects.empty() && added_objects.empty())
        return;

    char buf[4];
    std::string data;

    size_t buf_size_ao = added_objects.size();
    bool ao_rmv_1 = false;
    size_t bug_size_do = removed_objects.size();
    bool do_rmv_1 = false;
    // Handle removed objects
    writeU16((u8*)buf, ro.size());
    data.append(buf, 2);
    while (!removed_objects.empty()) {
        // Get object
        u16 id = removed_objects.front();
        ServerActiveObject* obj = m_env->getActiveObject(id);
        if (obj && obj->m_known_by_count > 0)
            obj->m_known_by_count--;
        client->m_known_objects.erase(id);
        // Add to data buffer for sending
        writeU16((u8*)buf, id);
        data.append(buf, 2);

        removed_objects.pop();
    }

    writeU16((u8*)buf, ao.size());
    data.append(buf, 2);
    while (!added_objects.empty()) {
        // Get object
        u16 id = added_objects.front();
        ServerActiveObject *obj = m_env->getActiveObject(id);
        added_objects.pop();
        client->m_known_objects.insert(id);
        obj->m_known_by_count++;
        if (!obj) {
            warningstream << FUNCTION_NAME << ": NULL object id=" << (int)id << std::endl;
            continue;
        }

        // Get object type
        u8 type = obj->getSendType();
        // Add to data buffer for sending
        writeU16((u8*)buf, id);
        data.append(buf, 2);
        writeU8((u8*)buf, type);
        data.append(buf, 1);

        data.append(serializeString32(obj->getClientInitializationData(client->net_proto_version)));
    }

    u16 SrvID = ServersNetworkObject->QueryThisServerID();

    NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD, data.size(), client->GetPlayerID_(), 0, ThisServID);
    pkt.putRawString(data.c_str(), data.size());

    Send(&pkt);

    verbosestream << "Server::SendActiveObjectRemoveAdd: " << removed_count << " removed, " << added_count << " added, " << "packet size is " << pkt.getSize() << std::endl;

    Send(&pkt);
}

void Server::SendActiveObjectMessages(uint16_t peer_id, const std::string &datas, bool reliable)
{
    NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES, datas.size(), peer_id, 0, ThisServID);
    pkt.putRawString(datas.c_str(), datas.size());

    if (!ServersNetworkObject->AreSlave)
        m_clients.send(pkt.getPeerId(), reliable ? clientCommandFactoryTable[pkt.getCommand()].channel : 1, &pkt, reliable);
    else
        Send(&pkt);
}

/*
 * NOTE: Useful for later.
 * if (obj->getType() == ACTIVEOBJECT_TYPE_PLAYER) { //A player is being initialized
 *   RemotePlayer *player = m_env->FindPlayerWithThisId(obj->getId());
 *   verbosestream << FUNCTION_NAME << ": Initialization->Player" << std::endl;
 *   if (!player) {
 *       warningstream << FUNCTION_NAME << ": Not known player: " << obj->getId() << std::endl;
 *       continue;
 *   }
 *
 *   //Let's make a packet with the initialization data
 *   NetworkPacket pkt_(0x77, 3); // 0x77: Single Packet AOM redirected to a specific player on the game
 *   std::string _init = obj->GetInitData(client->net_proto_version);
 *   verbosestream << FUNCTION_NAME << ": InitStringLen: " << _init.size() << std::endl;
 *
 *   std::string buff_;
 *   char bf[2];
 *   writeU8((u8*)bf, (u8)ServersNetworkObject->QueryThisServerID());
 *   buff_.append(bf, 1);
 *   writeU16((u8*)bf, player->player_id);
 *   buff_.append(bf, 2);
 *   buff_.append(_init);
 *
 *   pkt_.putRawString(buff_.c_str(), buff_.size());
 *
 *   Send(&pkt_);
 *   //continue;
 } *
 */

//END ObjectManagement

//BEGIN Crafting

void Server::UpdateCrafting(RemotePlayer *player)
{
    InventoryList *clist = player->inventory.getList("craft");
    if (!clist || clist->getSize() == 0)
        return;

    if (!clist->checkModified())
        return;

    // Get a preview for crafting
    ItemStack preview;
    InventoryLocation loc;
    loc.setPlayer(player->getName());
    std::vector<ItemStack> output_replacements;
    getCraftingResult(&player->inventory, preview, output_replacements, false, this);
    //m_env->getScriptIface()->item_CraftPredict(preview, player->getPlayerSAO(), clist, loc); // TODO: DO THIS

    InventoryList *plist = player->inventory.getList("craftpreview");
    if (plist && plist->getSize() >= 1) {
        // Put the new preview in
        plist->changeItem(0, preview);
    }
}

//END Crafting

//BEGIN Chat

void Server::notifyPlayer(const char *name, const std::wstring &msg) {
    if (!m_env)
        return;

    if (m_admin_nick == name && !m_admin_nick.empty()) {
        m_admin_chat->outgoing_queue.push_back(new ChatEventChat("", msg));
    }

    RemotePlayer *player = m_env->getPlayer(name);
    if (!player) {
        return;
    }

    if (player->getPeerId() != PEER_ID_INEXISTENT)
        SendChatMessage(player->getPeerId(), ChatMessage(msg));
}

//Obsolete.
void Server::handleChatInterfaceEvent(ChatEvent *evt)
{
    if (evt->type == CET_NICK_ADD) {
        // The terminal informed us of its nick choice
        m_admin_nick = ((ChatEventNick *)evt)->nick;
        /*if (!m_script->getAuth(m_admin_nick, NULL, NULL)) {
            errorstream << "You haven't set up an account." << std::endl
            << "Please log in using the client as '"
            << m_admin_nick << "' with a secure password." << std::endl
            << "Until then, you can't execute admin tasks via the console," << std::endl
            << "and everybody can claim the user account instead of you," << std::endl
            << "giving them full control over this server." << std::endl;
        }*/
    } else {
        assert(evt->type == CET_CHAT);
        handleAdminChat((ChatEventChat *)evt);
    }
}

std::wstring Server::handleChat(const std::string &name, std::wstring wmessage, bool check_shout_priv, RemotePlayer *player) {
    if (g_settings->getBool("strip_color_codes"))
        wmessage = unescape_enriched(wmessage);

    const bool sscsm_com = wmessage.find(L"/admin \x01SSCSM_COM\x01", 0) == 0;
    if (player && !sscsm_com) {
        switch (player->canSendChatMessage()) {
            case RPLAYER_CHATRESULT_FLOODING: {
                std::wstringstream ws;
                ws << L"You cannot send more messages. You are limited to "
                << g_settings->getFloat("chat_message_limit_per_10sec")
                << L" messages per 10 seconds.";
                return ws.str();
            }
            case RPLAYER_CHATRESULT_KICK:
                DenyAccess(player->getPeerId(), SERVER_ACCESSDENIED_CUSTOM_STRING,
                           "You have been kicked due to message flooding.");
                return L"";
            case RPLAYER_CHATRESULT_OK:
                break;
            default:
                FATAL_ERROR("Unhandled chat filtering result found.");
        }
    }

    if (m_max_chatmessage_length > 0 && wmessage.length() > m_max_chatmessage_length) {
        return L"Your message exceed the maximum chat message limit set on the server. It was refused. Send a shorter message";
    }

        auto message = trim(wide_to_utf8(wmessage));
    if (message.find_first_of("\n\r") != std::wstring::npos) {
        return L"Newlines are not permitted in chat messages";
    }

    // Run script hook, exit if script ate the chat message
    //if (m_script->on_chat_message(name, message)) {
    if (false) {
        if (!sscsm_com)
            actionstream << name << " issued command: " << message << std::endl;
        return L"";
    }

    // Line to send
    std::wstring line;
    // Whether to send line to the player that sent the message, or to all players
    bool broadcast_line = true;

    if (check_shout_priv && !checkPriv(name, "shout")) {
        line += L"-!- You don't have permission to shout.";
        broadcast_line = false;
    } else {
        line += utf8_to_wide(formatChatMessage(name, wide_to_utf8(wmessage)));
    }

    /*
     *	Tell calling method to send the message to sender
     */
    if (!broadcast_line)
        return line;

    /*
     *	Send the message to others
     */
    actionstream << "CHAT: " << wide_to_utf8(unescape_enriched(line)) << std::endl;

    ChatMessage chatmsg(line);

    SendChatMessage(PEER_ID_INEXISTENT, chatmsg);

    return L"";
}

//Obsolete
void Server::handleAdminChat(const ChatEventChat *evt) {
    std::string name = evt->nick;
    std::wstring wmessage = evt->evt_msg;

    std::wstring answer = handleChat(name, wmessage);

    // If asked to send answer to sender
    if (!answer.empty()) {
        m_admin_chat->outgoing_queue.push_back(new ChatEventChat("", answer));
    }
}

//END Chat

//BEGIN Misc

std::string Server::getStatusString()
{
    std::ostringstream os(std::ios_base::binary);

    os << "# MineStars ShootingStars 4.0 <MultiCraft 2.0.6>: ";
    // Uptime
    os << ", uptime= 0 [Unavailable data]";
    // Max lag estimate
    os << ", max_lag=" << (m_env ? m_env->getMaxLagEstimate() : 0);

    if (!g_settings->get("motd").empty())
        os << std::endl << "# Server: " << g_settings->get("motd");

    return os.str();
}

//END Misc

//Gaster would be happy.. i think...




