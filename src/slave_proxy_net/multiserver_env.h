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

#pragma once

#include <deque>
#include <unordered_map>
#include "../threading/thread.h"
#include "../network/networkprotocol.h"

class ServerNetworkEngine;
class SlaveServerObjects;
class NetworkPacket;
class SlaveServerPM;
class Server;

struct PlayerWithID {
	PlayerWithID(uint16_t ID, std::string OD, std::string ND, bool IO): ID(ID), old_DATA(OD), new_DATA(ND), isOld(IO) {};
	uint16_t ID;
	std::string old_DATA; //NOTE: Because of versions
	std::string new_DATA;
	bool isOld = false;
};

//Should be in the SlaveServer Object
//This will be useful to send updates to the slave server
class SlaveServerThread: public Thread {
	friend class SlaveServerPM;
public:
	void *run();
protected:
	SlaveServerThread(SlaveServerPM *jd, uint16_t s_id): JD(jd), S_ID(s_id), Thread(std::string("SlaveServerPM")) {};
	void preparePlayers(uint16_t playerid, std::deque<PlayerWithID> *aom, std::deque<NetworkPacket> packets);
private:
	SlaveServerPM *JD;
	uint16_t S_ID;
};

typedef std::unordered_map<uint16_t, uint16_t> ClientsSAOIDS;

class SlaveServerPM {
	friend class SlaveServerThread;
public:
	SlaveServerPM(ServerNetworkEngine *server, uint16_t ServerID);
	uint16_t ServerID = 0;
	void registerPlayerToSlave(session_t peer_id, uint16_t p_id); //Suscribe to all updates of the slave
	void unregisterPlayerFromSlave(session_t peer_id); //Unsubscribe to all updates from the slave
	void movePlayer(NetworkPacket *pkt); //Updates (Queued to be processed in a different thread)
	void SHUTDOWN(); //Kills the thread
	void MakeNrunThreads(); //PowersOn the thread
	std::string doActiveObjectMSGaboutMovements(const v3f &position, const v3f &rotation, const float updt, const uint16_t protocol_version);
	NetworkPacket sendAllPlayerMovements();
	void updatePlayerSaoList(NetworkPacket *pkt);
	
	SlaveServerObjects *SSO = nullptr;
private:
	Server *m_server = nullptr;
	std::unordered_map<session_t, uint16_t> Clients; //session_t: Peer ID, uint16_t: Player ID <Slave Server>
	std::unordered_map<uint16_t, ClientsSAOIDS> ClientsSaoId; //Updated ONLY by the slave {Determines how the player sees the other players SAO}
	std::mutex mutex;
	std::deque<NetworkPacket> Packets;
	SlaveServerThread *ServerThread = nullptr;
	ServerNetworkEngine *SNE = nullptr;
};

