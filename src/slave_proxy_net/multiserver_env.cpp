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

/* THIS WILL SEND & UPDATE PLAYER MOVEMENTS TO ALL PLAYER IN THE SAME NETWORK */

/*
   This will simulate an ActiveObjectMessages meaning with ONLY movements/interact details
    ____________________
   |                    |
   | TOSERVER_INTERACT  |
   | TOSERVER_PLAYERPOS |
   |____________________|>----#
                              |
      #-----------------------#
      |
      v
    Player -> Proxy < | ! | > Slave
                        ^    
                        |    
                        #---> Not sent to slave (Too heavy charge of bandwith)
*/

//May handle head object?

//Only send PlayerPOS every 2 secs
//TOSERVER_INTERACT Need to be sent into server whenever costs, to write all interactions to the slave

#include <deque>

#include "multiserver_env.h"
#include "../threading/thread.h"
#include "../network/networkpacket.h"
#include "../activeobject.h"
#include "../server.h"
#include "../serverenvironment.h"
#include "../ServerNetworkEngine.h"
#include "../constants.h"
#include "long_term_obj_manage.h"

/* SlaveServer */

SlaveServerPM::SlaveServerPM(ServerNetworkEngine *obj, uint16_t ServerID): SNE(obj), ServerID(ServerID) {
	//Get the Server object
	m_server = SNE->Server___;
	MakeNrunThreads();
}

//Suscribes player to the slave server, to receive any updates of it
void SlaveServerPM::registerPlayerToSlave(session_t peer_id, uint16_t p_id) {
	actionstream << "Registering player peer_id(" << peer_id << ") into a slave server (" << ServerID << ")" << std::endl;
	Clients[peer_id] = p_id;
}

void SlaveServerPM::unregisterPlayerFromSlave(session_t peer_id) {
	actionstream << "unRegistering player of peer_id(" << peer_id << ")" << " of the slave (" << ServerID << ")" << std::endl;
	if (Clients.find(peer_id) != Clients.end()) {
		u16 pid = Clients[peer_id];
		SNE->erasePlayerSAOid(ServerID, pid);
		Clients.erase(peer_id);
	}
}

void SlaveServerPM::movePlayer(NetworkPacket *pkt) {
	mutex.lock();
	Packets.push_back(*pkt);
	mutex.unlock();
}

//This function simulates an ActiveObjectMessage which will be sent to all members in the slave server as an update of the player
std::string SlaveServerPM::doActiveObjectMSGaboutMovements(const v3f &position, const v3f &rotation, const float updt, const uint16_t protocol_version) {
	std::ostringstream os(std::ios::binary);
	writeU8(os, AO_CMD_UPDATE_POSITION);
	
	if (protocol_version < 37)
		writeV3F(os, position + v3f(0.0f, BS, 0.0f), protocol_version);
	else
		writeV3F(os, position, protocol_version);
	
	if (protocol_version >= 37)
		writeV3F32(os, rotation);
	else
		writeF1000(os, rotation.Y);
	
	writeV3F(os, v3f(0.0f,0.0f,0.0f), protocol_version); //vel
	writeV3F(os, v3f(0.0f,0.0f,0.0f), protocol_version); //acc
	
	writeU8(os, true);
	writeU8(os, false);
	writeF(os, updt, protocol_version);
	
	return os.str();
}

void SlaveServerPM::MakeNrunThreads() {
	ServerThread = new SlaveServerThread(this, ServerID);
	ServerThread->start();
}

void SlaveServerPM::SHUTDOWN() {
	ServerThread->stop();
	delete ServerThread;
}

void SlaveServerPM::updatePlayerSaoList(NetworkPacket *pkt) {
	/*
	Structure
	[0] Command
	[2] PlayerID of updater
	[4] Count
	{
		[6+] Player SAO owner ID
		[8+] Player SAO ID seen by the player
	}
	*/
	ClientsSAOIDS _Clients;
	u16 PlayerID = 0;
	u16 Count = 0;
	u16 Sao = 0;
	*pkt >> PlayerID >> Count >> Sao; //Sao is used for updating AOM messages in engine
	
	//SNE->setPlayerSAOidOfPlayerReverse(ServerID, Sao, PlayerID);
	
	for (u16 i = 0; i < Count; i++) {
		u16 p_sao_owner_id;
		u16 p_sao_id;
		*pkt >> p_sao_owner_id >> p_sao_id;
		_Clients[p_sao_owner_id] = p_sao_id;
	}
	//Update
	ClientsSaoId[PlayerID] = _Clients;
}

//This makes an packet with ALL player movements possible to compress all movements into 1 packet
NetworkPacket SlaveServerPM::sendAllPlayerMovements() {
	u16 packet_count = Packets.size();
	/*
	Structure
	[0] Command
	[2] Player Count
	[...packet + 2(ID)]
	*/
	if (packet_count == 0)
		return NetworkPacket(0x0, 0, 0);
	
	//May copy
	std::deque<NetworkPacket> packets = Packets;
	
	NetworkPacket pkt(0x97, 0); //special command
	pkt << packet_count;
	while (!packets.empty()) {
		/*
		v3s32 pos
		v3s32 speed
		s32 FPT, FYW
		u8 FFOV
		u32 keyPressed;
		u8 wanted_range
		u8 key_pressed_got
		u8 using_new_version1 [FOV]
		u8 using_new_version2 [WANTED RANGE]
		*/
		NetworkPacket pkt_ = packets.front();
		packets.pop_front();
		pkt << Clients[pkt_.getPeerId()]; // PlayerID
		//Data
		v3s32 pos, speed;
		s32 PITCH, YAW = 0;
		u8 WANTED_RANGE, F_FOV = 0;
		u32 KP = 0;
		bool U_N_V1 = false;
		bool U_N_V2 = false;
		bool KP_G = false;
		
		pkt_ >> pos >> speed;
		pkt_ >> PITCH >> YAW;
		
		if (pkt_.getRemainingBytes() >= 4) {
			pkt_ >> KP;
			KP_G = true;
		}
		
		//Check if using new versions
		if (pkt_.getRemainingBytes() >= 1) {
			U_N_V1 = true;
			pkt_ >> F_FOV;
			if (pkt_.getRemainingBytes() >= 1) {
				U_N_V2 = true;
				pkt_ >> WANTED_RANGE;
			}
		}
		
		//Process
		pkt << pos << speed << PITCH << YAW << KP << F_FOV << WANTED_RANGE << (u8)KP_G << (u8)U_N_V1 << (u8)U_N_V2;
	}
	
	verbosestream << "Sending " << packet_count << " compressed packets into 1 to slave server {size=" << pkt.getSize() << "}" << std::endl;
	
	return pkt;
}

/* SlaveServerThread */

void SlaveServerThread::preparePlayers(uint16_t playerid, std::deque<PlayerWithID> *aom, std::deque<NetworkPacket> packets) {
	//This function will prepare data for sending
	//Already mutexed because of ::run()
	
	auto get_packet = [this, playerid, &aom] (NetworkPacket *pkt) {
		//Parse <we Only need pos>
		//rotation needs to be an v3f value
		v3s32 pos, ss;
		s32 pitch, yaw;
		*pkt >> pos >> ss >> pitch >> yaw;
		(void) ss;
		(void) pitch;
		v3f rotation = v3f(0.0f, yaw, 0.0f);
		v3f position((f32)pos.X / 100.0f, (f32)pos.Y / 100.0f, (f32)pos.Z / 100.0f);
		//make an update
		bool old = false;
		uint16_t protocol = JD->m_server->getProtocolOfThisPeer(pkt->getPeerId());
		std::string str = JD->doActiveObjectMSGaboutMovements(position, rotation, JD->m_server->getRecommendedSendInterval(), protocol); //This gets a old/new datastring version
		if (protocol < 37)
			old = true;
		PlayerWithID pwi = PlayerWithID(JD->SSO->GetObjectByPlayer(playerid, JD->ClientsSaoId[playerid][JD->Clients[pkt->getPeerId()]]), str, str, old);
		aom->push_back(pwi);
	};
	
	while (!packets.empty()) {
		//Packet
		NetworkPacket pkt = packets.front();
		packets.pop_front();
		//May skip own player
		if (JD->m_server->equalsPlayerIDwithPeerID(playerid, pkt.getPeerId()))
			continue;
		//Get packet by packet
		get_packet(&pkt);
	}
}

void *SlaveServerThread::run() {
	verbosestream << "Starting SlaveServer Thread for Player Movements" << std::endl;
	std::deque<PlayerWithID> aom; //Special for movements
	while (!stopRequested()) {
		//Process here
		//Send movements to everyone
		
		JD->mutex.lock();
		JD->SNE->SendRawToSlaveServer(JD->ServerID, JD->sendAllPlayerMovements());
		std::deque<NetworkPacket> packets = std::move(JD->Packets);
		JD->Packets.clear();
		JD->mutex.unlock();
		
		for (auto it = JD->Clients.begin(); it != JD->Clients.end(); ++it) {
			aom.clear();
			//Acquire data
			preparePlayers(it->second, &aom, packets);
			//Compile here the data
			std::string url_data;
			while (!aom.empty()) {
				url_data.clear();
				PlayerWithID AOM = aom.front();
				char idbuf[2];
				writeU16((u8*)idbuf, AOM.ID);
				url_data.append(idbuf, sizeof(idbuf));
				//Write data <Depends which version of protocol>
				if (AOM.isOld)
					url_data.append(serializeString16(AOM.old_DATA));
				else
					url_data.append(serializeString16(AOM.new_DATA));
				
				aom.pop_front();
			}
			if (url_data.empty())
				continue;
			JD->m_server->SendActiveObjectMessages(it->first, url_data, false);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	return 0;
}

/* NODEDEF MANAGEMENTS */

//moved to definitions_jumper[.h, .cpp]

/* Head Movements [Improves minus lag] */
//The slave server sends the player's head id to store here, then we can send a AOM message with movement/bone position [Don't send yaw/pitch messages to slave]















