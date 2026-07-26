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

//This will fork a new subserver with specified args, also with a free set of mods

#include "slave_server_creator.h"

#include "../ServerNetworkEngine.h" //Server Files formatting and data
#include "../util/serialize.h"
#include "../slave_proxy_net/randomchar.h"
#include "../network/address.h"
#include "log.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <signal.h>
#include <mutex>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

//registered_servers.mdf // mdf=Minestars.Data.File (Header are 9 bytes size: MINESTARS)

void SS_CREATOR::addIntoServerList(std::string name, std::string path, bool del) {
    std::ostringstream os(std::ios_base::binary); //Needed for data
    bool got = false;
    SNE->LOADFILE(std::string("/registered_servers.mdf"), os, got);
    std::unordered_map<std::string, std::string> servers;
    if (!del) {
        servers[name] = path;
    }
    if (got) {
        char BUFFER[240] = {0};
        std::string raw_data = os.str();
        //format
        //First 9 bytes will be ignored
        /*
        Format:
        [0-9] MineStars
        [9-11] Count
        [11-13] Size of Name
        [13-15] Size of path
        [**, **] Data
        */



        uint16_t read = 12;

        uint8_t count = (uint8_t)raw_data[11];

        if (!del) {
            for (uint8_t i = 0; i < count; i++) {
                uint16_t name_len = ((uint8_t)raw_data[read] << 8) | ((uint8_t)raw_data[read+1] << 0);
                uint16_t path_len = ((uint8_t)raw_data[read+2] << 8) | ((uint8_t)raw_data[read+1+2] << 0);
                memcpy(BUFFER, raw_data.data() + (read+2+2), name_len);
                std::string _name = std::string(BUFFER, name_len);
                //Overwrite buffer
                memcpy(BUFFER, raw_data.data() + (read+2+2+name_len), path_len);
                std::string _path = std::string(BUFFER, path_len);
                servers[_name] = _path;
                verbosestream << "SS_CREATOR: AddIntoServerList(): Found subServer: " << _name << " = " << _path << std::endl;
            }

            verbosestream << "SS_CREATOR: AddIntoServerList(): Creating new subServer: " << name << " = " << path << std::endl;

            servers[name] = path;
        } else {
            verbosestream << "SS_CREATOR: AddIntoServerList(): Skipping " << name << " as it will be deleted" << std::endl;
        }
    }

    verbosestream << "SS_CREATOR: AddIntoServerList(): Going to save all servers..." << std::endl;

    std::ostringstream nfile(std::ios_base::binary);
    nfile << std::string("MineStars"); //raw
    char _buf[2];
    std::string _bufh;
    _bufh.reserve(2);
    writeU16((uint8_t*)&_buf[0], (uint16_t)servers.size()); //If servers size are 1, its new the database
    _bufh.append(_buf, 2);
    nfile << _bufh;

    for (const auto &i: servers) {
        //Sizes first
        //we will use _buf
        writeU16((uint8_t*)&_buf[0], (uint16_t)i.first.size());
        _bufh.clear();
        _bufh.append(_buf, 2);
        nfile << _bufh;
        _bufh.clear();
        writeU16((uint8_t*)&_buf[0], (uint16_t)i.second.size());
        _bufh.append(_buf, 2);
        //Strings entries
        nfile << i.first;
        nfile << i.second;

        Servers[i.first] = i.second;
    }

    //Save into file
    std::string main_path = SNE->minestars_folder_path + "/registered_servers.mdf";
    SNE->SAVEFILE(main_path, nfile.str());
}
//when loading server entire must load subservers tho
bool SS_CREATOR::Destroy(std::string name) {
    //Only unlink the server from the paths, but don't delete the data'
    if (Servers.find(name) != Servers.end()) {
        addIntoServerList(name, "", true);
        return true;
    } else {
        errorstream << "SS_CREATOR: Destroy: Unknown subserver '" << name << "'" << std::endl;
        return false;
    }
}

bool SS_CREATOR::Create(std::string name) {
    if (Servers.find(name) == Servers.end()) {
        //Lets create the full path, then we will save into registered_servers.mdf
        std::string path = SNE->minestars_folder_path;
        std::string worldpath = path + "/Worlds/" + name;
        //Save it
        addIntoServerList(name, worldpath, false);
        //Create folders and data for server
        SNE->CHECKFOLDER("/Worlds");
        SNE->CHECKFOLDER("/Worlds/" + name);
        return true;
    } else {
        errorstream << "SlaveServerCreator::Create(): Server already exists" << std::endl;
        return false;
    }
}

bool SS_CREATOR::Exists(std::string name) {
    return Servers.find(name) != Servers.end();
}

bool SS_CREATOR::StopServ(std::string name) {
    if (ActiveServers.find(name) == ActiveServers.end())
        return false;
    //Only send kill fork and its done
    pid_t pid = ActiveServers.at(name).pid;
    kill(pid, SIGKILL);
    return true;
}

void SS_CREATOR::setActiveServer(std::string name, pid_t pid) {
    ActiveServer DATA;
    DATA.pid = pid;
    ActiveServers[name] = DATA;
}

//Here the magic starts

#include "../server.h"
#include "threading_jit.h"
//#include "../content/subgames.h"

bool SS_CREATOR::Start(std::string name) {
    if (ActiveServers.find(name) == ActiveServers.end()) {
        actionstream <<FUNCTION_NAME<<": Running server: " << name << std::endl;
        ServerSocket *service = new ServerSocket(name, SNE->minestars_folder_path + "/Worlds/" + name, this);
        bool did_init = service->initialize(SNE);
        if (!did_init) {
            errorstream << "Failure to initialize server: " << name << std::endl;
            return false;
        }
        //Save
        ServerThreads[name] = service;
        service->start();
        return true;
    } else {
        warningstream << FUNCTION_NAME << ": Trying to start an active server" << std::endl;
        return false;
    }
}

bool ServerSocket::initialize(ServerNetworkEngine *_SNE) {
    SNE = _SNE;
        //Load worldpath
        //It are already loaded in cache, else, segmentation fault
        //WorldSpec world(Servers.at(name), name, SNE->Server___->getGameSpec().id);
        //getGameSpec
        //The GameSpec might will be equal, soon ill add special configs to handle this.

        //FIXME: Too unstable to use due of the use of ports and addresses (Not good for a luanti-minetest-multicraft host provider...)
        /*uint16_t socket_port = randomNumber(35000, 36000);
        if (OccupiedPorts.find(socket_port) != OccupiedPorts.end()) {
            while (true) {
                socket_port = randomNumber(35000, 36000);
                if (OccupiedPorts.find(socket_port) == OccupiedPorts.end()) {
                    break;
                }
            }
        }

        Address bind_addr(0, 0, 0, 0, socket_port);
        bind_addr.Resolve("0,0,0,0");

        Server serveur(Servers.at(name), SNE->Server___->getGameSpec(), false, bind_addr, true);

        //Some configurations to link to the main server, as this new server will be an fork (Like a completely new server with different port)

        serveur.ServersNetworkObject->AreSlave = true;
        serveur.ServersNetworkObject->SetTSID(SNE->getCountofServers()+1);
        serveur.ServersNetworkObject->ProxyAddressAdded = true;
        serveur.ServersNetworkObject->ProxyAddress = SNE->Server___->m_bind_addr;

        SNE->RegisterProxyServer(bind_addr, SNE->getCountofServers()+1, true, );

        fork();

        try {
            serveur.start();
            bool &kill = *porting::signal_handler_killstatus();
            dedicated_server_loop(serveur, kill);
        } catch (const ModError &e) {
            errorstream << "ModError: " << e.what() << std::endl;
        } catch (const ServerError &e) {
            errorstream << "ServerError: " << e.what() << std::endl;
        }*/
        //Using sockets instead
        //NOTE: THIS ONLY CREATES THE SOCKET, THIS DOENS'T LISTEN TO THE SOCKET, THE SLAVE SERVER DOES LISTEN!
        int socket_id = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socket_id == -1) {
            errorstream << "Unable to set up socket for server: " << name_serv << std::endl;
            return false;
        }
        std::string _SOCKDIR = "/tmp/stream_" + name_serv + ".sock"; //Used in the center of lambda
        struct sockaddr_un addr;
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, _SOCKDIR.c_str(), sizeof(addr.sun_path) - 1);
        addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
        if (bind(socket_id, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            errorstream << "Unable to bind to socket for server: " << name_serv << std::endl;
            return false;
        }
        //Run the server, on different thread but same arguments
        actionstream << FUNCTION_NAME << ": Going to register server!" << std::endl;

        ID = SNE->getCountofServers()+1;
        sock_dir = _SOCKDIR;
        sID = socket_id;

        SNE->RegisterProxyServer(Address(), ID, false, 0);
        //Will connect via socket
        ActiveServersStructure *as = SNE->getAS(ID); //Acts like a client
        as->SocketConn = true;
        as->SocketDir = _SOCKDIR;

        ServerData = as;

        //Update servers base
        SNE->InitializeServer(ID);

        //Listen & Run [In the new thread]
        //Just exit gracefully and let main thread run the new configured thread
        return true;
}

#include "../settings.h"
#include "main.h"

void *ServerSocket::run() {
    warningstream <<  FUNCTION_NAME << ": RUNNING SERVER IN SEPARATE THREAD!" << std::endl;
    //Here the server runs
    //TODO: May try fork

    ServerData->active.store(true);
    pid_t pid = fork();

    if (pid != 0) {
        return nullptr; // Should not continue on a parent thread
    }

    actionstream <<FUNCTION_NAME << ": Pid: " << pid << std::endl;

    if (ServerData == nullptr) {
        errorstream << FUNCTION_NAME << ": Server are not initialized correctly!" << std::endl;
        return nullptr;
    }

    //Log files & stream logs must don't conflict
    SLAVE_INSTANCE.store(true); // Log must be an buffer that might save on a logfile.
    SLAVE_INSTANCE_2.store(true);
    FileLogOutput file_log_output;
    g_logger.removeOutput(&file_log_output);
    file_log_output.setFile(std::string(path + "/service.log"), 640000000);
    g_logger.addOutputMaxLevel(&file_log_output, LL_INFO);

    //This is running in a different thread, but functions must be different for no collision between variables
    Server serveur(path, SNE->Server___->getGameSpec(), false, Address(), true);
    //Some configurations to link to the main server, as this new server will be an fork (Like a completely new server with different port)

    link->setActiveServer(name_serv, pid);

    serveur.SocketConn = true;
    serveur.SocketDir = sock_dir;
    serveur.SocketID = sID; //Transfer ownership [Acts like a server]
    serveur.ServersNetworkObject->AreSlave = true;
    serveur.ServersNetworkObject->SetTSID(SNE->getCountofServers()+1);
    serveur.ServersNetworkObject->ProxyAddressAdded = true; //Socket
    serveur.TAOM = nullptr;
    serveur.MAINSERVEURE = nullptr;
    //serveur.ServersNetworkObject->ProxyAddress = SNE->Server___->m_bind_addr;
    try {
        warningstream<<"Running subServer: " << ServerData->ID << std::endl;
        serveur.start();
        bool &kill = *porting::signal_handler_killstatus();
        dedicated_server_loop(serveur, kill);
    } catch (const ModError &e) {
        errorstream << "ModError: " << e.what() << std::endl;
    } catch (const ServerError &e) {
        errorstream << "ServerError: " << e.what() << std::endl;
    }
}
