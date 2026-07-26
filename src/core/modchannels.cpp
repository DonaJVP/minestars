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
#include "../modchannels.h"
#include "../script/scripting_server.h"
#include "../ServerNetworkEngine.h"
#include "../log.h"
#include "../addons/cblks_def.hpp"

//NOTE: For slave & main mods communication, theres already an API on ServersNetworkEngine which allows it and now is on lua side, so ModChannels are unsupported

bool Server::joinModChannel(const std::string &channel)
{
    if (ServersNetworkObject->AreSlave) {
        errorstream << "Unsupported joinModChannel on SlaveServer for: " << channel << std::endl;
        return false;
    }
    return m_modchannel_mgr->joinChannel(channel, PEER_ID_SERVER) && m_modchannel_mgr->setChannelState(channel, MODCHANNEL_STATE_READ_WRITE);
}

bool Server::leaveModChannel(const std::string &channel)
{
    if (ServersNetworkObject->AreSlave) {
        errorstream << "Unsupported leaveModChannel on SlaveServer for: " << channel << std::endl;
        return false;
    }
    return m_modchannel_mgr->leaveChannel(channel, PEER_ID_SERVER);
}

bool Server::sendModChannelMessage(const std::string &channel, const std::string &message)
{
    if (ServersNetworkObject->AreSlave) {
        errorstream << "Unsupported sendModChannelMessage on SlaveServer for: " << channel << std::endl;
        return false;
    }
    if (!m_modchannel_mgr->canWriteOnChannel(channel))
        return false;

    broadcastModChannelMessage(channel, message, PEER_ID_SERVER);
    return true;
}

ModChannel* Server::getModChannel(const std::string &channel)
{
    if (ServersNetworkObject->AreSlave) {
        errorstream << "Unsupported getModChannel on SlaveServer for: " << channel << std::endl;
        return nullptr;
    }
    return m_modchannel_mgr->getModChannel(channel);
}

using _HELPER0 = void*(*)(const std::string, const std::string, const std::string);

void Server::broadcastModChannelMessage(const std::string &channel, const std::string &message, session_t from_peer)
{
    if (ServersNetworkObject->AreSlave) {
        errorstream << "Unsupported broadcastModChannelMessage on SlaveServer for: " << channel << std::endl;
        return;
    }
    const std::vector<u16> &peers = m_modchannel_mgr->getChannelPeers(channel);
    if (peers.empty())
        return;

    if (message.size() > STRING_MAX_LEN) {
        warningstream << "ModChannel message too long, dropping before sending "
        << " (" << message.size() << " > " << STRING_MAX_LEN << ", channel: "
        << channel << ")" << std::endl;
        return;
    }

    std::string sender;
    if (from_peer != PEER_ID_SERVER) {
        sender = getPlayerName(from_peer);
    }

    NetworkPacket resp_pkt(TOCLIENT_MODCHANNEL_MSG,
                           2 + channel.size() + 2 + sender.size() + 2 + message.size());
    resp_pkt << channel << sender << message;
    for (session_t peer_id : peers) {
        // Ignore sender
        if (peer_id == from_peer)
            continue;

        Send(peer_id, &resp_pkt);
    }

    if (from_peer != PEER_ID_SERVER) {
        (reinterpret_cast<_HELPER0>(AddonsCallbacks[FIXED_CALLBACK_ADDRESS])
        //m_script->on_modchannel_message(channel, sender, message);
    }
}
