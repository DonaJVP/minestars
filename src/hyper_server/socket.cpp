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

#include <cstdint>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "socket.h"

#include "../log.h" // Log
#include "../threading/thread.h" // Threading methods for Receive data
#include "../network/networkpacket.h" // Needed to compile NetworkPacket s

class __RecvThreadData: public Thread {
public:
    __RecvThreadData(): Thread("RecvThread") {};
    NetworkPacket AcquirePacket_FRONT() {
        _mutex.lock();

        _mutex.unlock();
    }
    void setSocketNumb(uint16_t sock) { _socket = sock; }
    void *run() {
        while (!stopRequested()) {
           // sockaddr_in client_addr{};
            //socklen_t client_addr_len = sizeof(client_addr);
            //recvfrom(sockfd, buffer, size of buffer, proto, (struct sockaddr*)&client_addr, &client_addr_len);

            sockaddr_in cadr{};
            socklen_t cadrl = sizeof(cadr);

            recvfrom(_socket, &_chars[0], UINT16_MAX, 0, (struct sockaddr*)&cadr, &cadrl); //Receive the data which the slave server sent, just keep it within milliseconds.

            //First 2 bytes are command
            //The 2-4 bytes are size data (16bit)
            //The rest might be data or neither command again

            if (sizeof(_chars) == 0)
                continue; //Discard

            uint16_t szs = _chars[2];
            if (szs > UINT16_MAX) {
                errorstream << "Packet Receive Thread [SlaveSent]: Size are bigger than the limit: " << UINT16_MAX << " bytes" << std::endl;
                continue;
            }

            uint16_t _iterator = 0;
            _mutex.lock();
            while (_iterator < UINT16_MAX) { //+2
                NetworkPacket pkt;
                _iterator += _chars[_iterator];
                pkt.putRawPacket(_chars[_iterator+2], _chars[_iterator], 1);
                Packets.push_back(pkt);
            }
            _mutex.unlock();
        }
    };
private:
    std::mutex _mutex;
    int _socket = 0;
    uint8_t _chars[UINT16_MAX] = {};
    std::deque<NetworkPacket> Packets; //Engine will compile all data which be a result in a NetworkPacket
};

void SlaveServerSocket::Connect(std::string addr, uint16_t port) {
    //May make a socket
    //UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    _socket = sockfd;
    if (sockfd == -1) {
        errorstream << "SlaveServerSocket::Connect(): Unable to make a socket for slave server, unknown error." << std::endl;
        return;
    }
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    rawserv = server_addr;
    //Bind
    bool status = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr) == -1);
    if (status) {
        errorstream << "SlaveServerSocket::Connect(): Unable to bind socket!";
        close(sockfd);
        return;
    }
    __RecvThreadData *_thr = new __RecvThreadData();
    _thr->setName("RecvThread");
    _thr->setSocketNumb(sockfd);
    _thr->start();
    //The thread reads all received data, as the time passes, the Receive thread will queue the data
}

void SlaveServerSocket::Send(NetworkPacket pkt) {
    //Dissasemble packet
    uint8_t rwd[UINT16_MAX] = pkt.disassemble_pkt();
    sendto(_socket, rwd, sizeof(rwd), 0, (struct sockaddr*)&rawserv, sizeof(rawserv));
}

