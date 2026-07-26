/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, <donatto555@gmail.com>
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

#include <deque>
#include <mutex>
#include <atomic>

#include "network/networkpacket.h"
#include "server.h"

#pragma once

class MovementQueue {
public:
    void Load(NetworkPacket pkt) {
        if (Using.load()) {
            Mutex.lock();
            _Packets.push_back(pkt);
            Mutex.unlock();
        } else {

        }
    }
private:
    std::atomic<bool> Using;
    std::mutex Mutex;
    std::deque<NetworkPacket> _Packets;
};
