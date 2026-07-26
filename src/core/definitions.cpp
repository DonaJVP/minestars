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
#include "../itemdef.h"
#include "../nodedef.h"
#include "../log.h"
#include "../network/networkpacket.h"

void Server::SendItemDef(uint16_t peer_id, IItemDefManager *itemdef, u16 protocol_version) {
    NetworkPacket pkt(TOCLIENT_ITEMDEF, 0, peer_id);
    /*
     *	u16 command
     *	u32 length of the next item
     *	zlib-compressed serialized ItemDefManager
     */
    std::ostringstream tmp_os(std::ios::binary);
    itemdef->serialize(tmp_os, protocol_version);
    std::ostringstream tmp_os2(std::ios::binary);
    compressZlib(tmp_os.str(), tmp_os2);
    pkt.putLongString(tmp_os2.str());

    // Make data buffer
    verbosestream << "Server: Sending item definitions to id(" << peer_id
    << "): size=" << pkt.getSize() << std::endl;

    Send(&pkt);
}

void Server::SendNodeDef(uint16_t peer_id, const NodeDefManager *nodedef, u16 protocol_version) {
    NetworkPacket pkt(TOCLIENT_NODEDEF, 0, peer_id);
    /*
     *	u16 command
     *	u32 length of the next item
     *	zlib-compressed serialized NodeDefManager
     */
    std::ostringstream tmp_os(std::ios::binary);
    nodedef->serialize(tmp_os, protocol_version);
    std::ostringstream tmp_os2(std::ios::binary);
    compressZlib(tmp_os.str(), tmp_os2);

    pkt.putLongString(tmp_os2.str());

    // Make data buffer
    verbosestream << "Server: Sending node definitions to id(" << peer_id << "): size=" << pkt.getSize() << std::endl;
    Send(&pkt);
}
