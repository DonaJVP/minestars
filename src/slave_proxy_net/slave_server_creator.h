/*
 M *ineStars - MultiCraft - Minetest/Luanti
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

#include <string>
#include <unordered_map>
#include <cstdint>
#include "../threading/thread.h"
#include "../network/networkpacket.h"
#include <deque>



struct ActiveServer {
    pid_t pid;
};

class ServerNetworkEngine;
class ActiveServersStructure;
class SS_CREATOR;

//Will control a entire server, the chat, movements, etc
class ServerSocket: public Thread {
public:
    ServerSocket(std::string name, std::string path, SS_CREATOR *link): Thread("Service-" + name), name_serv(name), path(path), link(link) {};
    bool initialize(ServerNetworkEngine *_SNE); //Data to be used
    void *run();
private:
    ServerNetworkEngine *SNE = nullptr; //Needed to directly load packets, as the slave will send big packets (Not compressed)
    ActiveServersStructure *ServerData = nullptr;
    uint16_t ID;
    uint16_t sID;
    std::string name_serv;
    std::string sock_dir;
    std::string path;
    SS_CREATOR *link = nullptr;
};

class SS_CREATOR {
public:
    SS_CREATOR(ServerNetworkEngine *SNE): SNE(SNE) {};
    bool Create(std::string name); //Only name required, world path is optional, it will be paused in ServerNetworkEngine
    bool StopServ(std::string name); //Identifier are name
    bool Exists(std::string name);
    bool Destroy(std::string name);
    bool Start(std::string name);
    std::unordered_map<std::string, std::string> getServersList() { return Servers; }
    std::unordered_map<std::string, ActiveServer> getActiveServersList() { return ActiveServers; }
    void setActiveServer(std::string name, pid_t pid);
private:
    std::unordered_map<std::string, std::string> Servers; //First arg: name, Second arg: path
    ServerNetworkEngine *SNE = nullptr; //Must be added in the SNE
    void addIntoServerList(std::string name, std::string path, bool del);
    std::unordered_map<std::string, ServerSocket*> ServerThreads;
    std::unordered_map<std::string, ActiveServer> ActiveServers;
};
