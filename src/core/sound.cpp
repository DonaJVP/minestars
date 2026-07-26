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
#include "../ServerNetworkEngine.h"
#include "../remoteplayer.h"
#include "../network/networkpacket.h"
#include "../network/peerhandler.h"
#include "../server/player_sao.h"
#include <cstdint>

int32_t Server::nextSoundId()
{
    int32_t ret = m_next_sound_id;
    if (m_next_sound_id == INT32_MAX)
        m_next_sound_id = 0; // signed overflow is undefined
    else
        m_next_sound_id++;
    return ret;
}

int32_t Server::playSound(const SimpleSoundSpec &spec, const ServerSoundParams &params, bool ephemeral) {
    if (!ServersNetworkObject->AreSlave) {
        // Find out initial position of sound
        bool pos_exists = false;
        v3f pos = params.getPos(m_env, &pos_exists);
        // If position is not found while it should be, cancel sound
        if(pos_exists != (params.type != ServerSoundParams::SSP_LOCAL))
            return -1;

        // Filter destination clients
        std::vector<session_t> dst_clients;
        if (!params.to_player.empty()) {
            RemotePlayer *player = m_env->getPlayer(params.to_player.c_str());
            if(!player){
                infostream<<"Server::playSound: Player \""<<params.to_player
                <<"\" not found"<<std::endl;
                return -1;
            }
            if (player->getPeerId() == PEER_ID_INEXISTENT) {
                infostream<<"Server::playSound: Player \""<<params.to_player
                <<"\" not connected"<<std::endl;
                return -1;
            }
            dst_clients.push_back(player->getPeerId());
        } else {
            std::vector<session_t> clients = m_clients.getClientIDs();

            for (const session_t client_id : clients) {
                RemotePlayer *player = m_env->getPlayer(client_id);
                if (!player)
                    continue;
                if (!params.exclude_player.empty() &&
                    params.exclude_player == player->getName())
                    continue;

                PlayerSAO *sao = player->getPlayerSAO();
                if (!sao)
                    continue;

                if (pos_exists) {
                    if(sao->getBasePosition().getDistanceFrom(pos) >
                        params.max_hear_distance)
                        continue;
                }
                dst_clients.push_back(client_id);
            }
        }

        if(dst_clients.empty())
            return -1;

        // Create the sound
        s32 id;
        ServerPlayingSound *psound = nullptr;
        if (ephemeral) {
            id = -1; // old clients will still use this, so pick a reserved ID
        } else {
            id = nextSoundId();
            // The sound will exist as a reference in m_playing_sounds
            m_playing_sounds[id] = ServerPlayingSound();
            psound = &m_playing_sounds[id];
            psound->params = params;
            psound->spec = spec;
        }

        float gain = params.gain * spec.gain;
        NetworkPacket pkt(TOCLIENT_PLAY_SOUND, 0);
        NetworkPacket legacypkt(TOCLIENT_PLAY_SOUND, 0, PEER_ID_INEXISTENT, 32);
        pkt << id << spec.name << gain
        << (u8) params.type << pos << params.object
        << params.loop << params.fade << params.pitch
        << ephemeral;
        legacypkt << id << spec.name << gain
        << (u8) params.type << pos << params.object
        << params.loop << params.fade;

        bool as_reliable = !ephemeral;
        bool play_sound = gain > 0;

        for (const u16 dst_client : dst_clients) {
            const u16 protocol_version = m_clients.getProtocolVersion(dst_client);
            if (!play_sound && protocol_version < 32)
                continue;
            if (psound)
                psound->clients.insert(dst_client);

            if (protocol_version >= 37)
                m_clients.send(dst_client, 0, &pkt, as_reliable);
            else
                m_clients.send(dst_client, 0, &legacypkt, as_reliable);
        }
        return id;
    } else {
        // Find out initial position of sound
        bool pos_exists = false;
        v3f pos = params.getPos(m_env, &pos_exists);
        // If position is not found while it should be, cancel sound
        if (pos_exists != (params.type != ServerSoundParams::SSP_LOCAL))
            return -1;

        std::vector<u16> tosend;
        u16 counttosend = 0;

        if (!params.to_player.empty()) {
            RemotePlayer *player = m_env->getPlayer(params.to_player.c_str());
            if (!player) {
                infostream << "Server::playSound: Player \"" << params.to_player << "\" not found" << std::endl;
                return -1;
            }
            if (player->player_id == 0) {
                infostream << "Server::playSound: Player \"" << params.to_player << "\" not connected"<<std::endl;
                return -1;
            }
            tosend.push_back(player->player_id);
            counttosend++;
        } else {
            ClientDataTable.Lock();
            for (auto it = ClientDataTable.GetRawMap()->begin(); it != ClientDataTable.GetRawMap()->end(); ++it) {
                RemotePlayer *player = m_env->getPlayer(it->first);
                if (!player)
                    continue;
                if (!params.exclude_player.empty() && params.exclude_player == player->getName())
                    continue;

                PlayerSAO *sao = player->getPlayerSAO();
                if (!sao)
                    continue;

                if (pos_exists) {
                    if (sao->getBasePosition().getDistanceFrom(pos) > params.max_hear_distance)
                        continue;
                }
                tosend.push_back(it->first);
                counttosend++;
            }
            ClientDataTable.unLock();
        }

        if (tosend.empty())
            return -1;

        // Create the sound
        s32 id;
        ServerPlayingSound *psound = nullptr;
        if (ephemeral) {
            id = -1; // old clients will still use this, so pick a reserved ID
        } else {
            id = nextSoundId();
            // The sound will exist as a reference in m_playing_sounds
            m_playing_sounds[id] = ServerPlayingSound();
            psound = &m_playing_sounds[id];
            psound->params = params;
            psound->spec = spec;
        }

        float gain = params.gain * spec.gain;
        NetworkPacket pkt(TOCLIENT_PLAY_SOUND, 0, PEER_ID_INEXISTENT, ServersNetworkObject->QueryThisServerID());
        //make table [u16 pcount, u16 .. playersid ..]
        pkt << counttosend;
        for (u16 clid : tosend) {
            pkt << clid;
            if (psound)
                psound->clients_int16.insert(clid);
        }
        pkt << id << spec.name << gain << (u8) params.type << pos << params.object << params.loop << params.fade << params.pitch << ephemeral;
        Send(&pkt);
        return id;
    }
}

void Server::stopSound(int32_t handle)
{
    if (!ServersNetworkObject->AreSlave) {
        // Get sound reference
        std::unordered_map<int32_t, ServerPlayingSound>::iterator i = m_playing_sounds.find(handle);
        if (i == m_playing_sounds.end())
            return;

        ServerPlayingSound &psound = i->second;

        NetworkPacket pkt(TOCLIENT_STOP_SOUND, 4);
        pkt << handle;

        for (std::unordered_set<session_t>::const_iterator si = psound.clients.begin(); si != psound.clients.end(); ++si) {
            // Send as reliable
            m_clients.send(*si, 0, &pkt, true);
        }
        // Remove sound reference
        m_playing_sounds.erase(i);
    } else {
        // Get sound reference
        std::unordered_map<int32_t, ServerPlayingSound>::iterator i = m_playing_sounds.find(handle);
        if (i == m_playing_sounds.end())
            return;

        ServerPlayingSound &psound = i->second;

        //Send to main proxy server to stop the sound handle

        NetworkPacket pkt(TOCLIENT_STOP_SOUND, 4 + 1 + 2, PEER_ID_INEXISTENT, 0, ServersNetworkObject->QueryThisServerID());
        pkt << handle;
        pkt << (u16)psound.clients_int16.size();

        for (std::unordered_set<u16>::const_iterator si = psound.clients_int16.begin(); si != psound.clients_int16.end(); ++si) {
            // Send as reliable
            pkt << *si; //i think it is an u16....
        }
        Send(&pkt);
        // Remove sound reference
        m_playing_sounds.erase(i);
    }
}

void Server::fadeSound(int32_t handle, float step, float gain)
{
    if (!ServersNetworkObject->AreSlave) {
        // Get sound reference
        std::unordered_map<int32_t, ServerPlayingSound>::iterator i = m_playing_sounds.find(handle);
        if (i == m_playing_sounds.end())
            return;

        ServerPlayingSound &psound = i->second;
        psound.params.gain = gain;

        NetworkPacket pkt(TOCLIENT_FADE_SOUND, 4);
        pkt << handle << step << gain;

        // Backwards compability
        bool play_sound = gain > 0;
        ServerPlayingSound compat_psound = psound;
        compat_psound.clients.clear();

        NetworkPacket compat_pkt(TOCLIENT_STOP_SOUND, 4);
        compat_pkt << handle;

        for (std::unordered_set<uint16_t>::iterator it = psound.clients.begin();
             it != psound.clients.end();) {
            if (m_clients.getProtocolVersion(*it) >= 32) {
                // Send as reliable
                m_clients.send(*it, 0, &pkt, true);
                ++it;
            } else {
                compat_psound.clients.insert(*it);
                // Stop old sound
                m_clients.send(*it, 0, &compat_pkt, true);
                psound.clients.erase(it++);
            }
             }

             // Remove sound reference
             if (!play_sound || psound.clients.empty())
                 m_playing_sounds.erase(i);

        if (play_sound && !compat_psound.clients.empty()) {
            // Play new sound volume on older clients
            playSound(compat_psound.spec, compat_psound.params);
        }
    } else {
        std::unordered_map<int32_t, ServerPlayingSound>::iterator i = m_playing_sounds.find(handle);
        if (i == m_playing_sounds.end())
            return;
        ServerPlayingSound &psound = i->second;
        psound.params.gain = gain;

        NetworkPacket pkt(TOCLIENT_FADE_SOUND, 4, PEER_ID_INEXISTENT, 0, ServersNetworkObject->QueryThisServerID());

        pkt << (uint16_t)psound.clients_int16.size();
        for (std::unordered_set<uint16_t>::iterator it = psound.clients_int16.begin(); it != psound.clients_int16.end();) {
            pkt << (uint16_t)*it;
        }

        pkt << handle << step << gain;
    }
}
