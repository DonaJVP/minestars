/*
 MineStars - MultiCraft - Minetest/Luanti
 Copyright (C) 2025 Logiki, <donatto555@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

//This module allows the main server create subservers with the same modset and different world, mainly configured in lua environment

class __RecvThreadData;
class NetworkPacket;

class SlaveServerSocket {
public:
    SlaveServerSocket() = default;
    void setPort(uint16_t port);
    void Connect(std::string addr, uint16_t port);
    //void Receive(char buffer[32768]); //Buffer must be big and limited
    void Send(NetworkPacket pkt);
    NetworkPacket AcquirePacket_FRONT();
private:
    uint16_t port;
    std::string addr;
    uint16_t _socket;
    sockaddr_in rawserv;
    __RecvThreadData *RecvThread = nullptr;
};

/*
 8
 16
 32
 64
 128
 256
 512
 1024
 2048
 4096
 8192
 16384
 32768
 */
