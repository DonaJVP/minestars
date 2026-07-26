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

#include "../threading/thread.h"
#include "../exceptions.h"
#include <mutex>
#include <deque>

class ClientNotFoundException : public BaseException
{
public:
    ClientNotFoundException(const char *s):
    BaseException(s)
    {}
};

class NetworkPacket;
class Server;

// NetworkCompressor [Slave Use]

class NetworkCompressor : public Thread {
public:
    NetworkCompressor(Server *server): Thread("Network[Compressor]"), m_server(server) {}
    void *run();
    void queuePacket(NetworkPacket pkt);
private:
    Server *m_server;
    std::mutex mutex;
    std::deque<NetworkPacket> queue_;
};

// Network Controller

class NetworkThread : public Thread {
public:
    NetworkThread(Server *server): Thread("Network[Main]"), m_server(server){}
    void *run();
    NetworkPacket getPacket();
    std::atomic<int> m_parent_skt;
    void queue(NetworkPacket pkt);
    void lock() {_mutex.lock();}
    void unlock() {_mutex.unlock();}
private:
    Server *m_server = nullptr;

    //Socket
    std::deque<NetworkPacket> packets;
    std::mutex _mutex;
    std::deque<NetworkPacket> packets_ts;
};
