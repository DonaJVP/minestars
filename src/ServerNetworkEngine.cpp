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

#include "ServerNetworkEngine.h"

#include "debug.h"
#include "log.h"
#include "media.h"
#include "script/scripting_server.h"
#include "server.h"
#include "serverenvironment.h"
#include "network/mt_connection.h"
#include "network/networkprotocol.h"
#include "network/serveropcodes.h"
#include "constants.h"
#include "remoteplayer.h"
#include "server/player_sao.h"
#include "slave_proxy_net/multiserver_env.h"
#include "slave_proxy_net/definitions_jumper.h"
#include "slave_proxy_net/threading_jit.h"
#include "slave_proxy_net/long_term_obj_manage.h"
#include "slave_proxy_net/verif.h"
#include "slave_proxy_net/slave_server_creator.h"
#include "util/sha1.h"
#include "util/base64.h"
//#include "core/lib/mt_map.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define GET_SRV_ID                \
ActiveServersStructure *SERVER__;  \
uint16_t SrvID = 0;                \
*pkt >> SrvID;                     \
SERVER__ = StoredServers.at(SrvID); \
                                  \

/* ServerNetworkEngine */

ServerNetworkEngine::ServerNetworkEngine(ServerEnvironment *env, Server *Serv, const std::string &path): m_env(env), Server___(Serv),
Slave_ItemDef(new SlaveItemDef()) {
	Slave_ItemDef->setSNE(this);
	Slave_ItemDef->set_itemdef(Server___->getWritableItemDefManager());
	Slave_ItemDef->set_nodedef(Server___->getWritableNodeDefManager());
	
	LOAD_FILESYSTEM(path);
	setSNEF(this);
	setSNE(this);
	runMMS();

	SSC = new SS_CREATOR(this);
}

void ServerNetworkEngine::FreezeServer() {
	Server___->DEFINITIONS_EXECUTION.lock();
}

void ServerNetworkEngine::unFreezeServer() {
	Server___->DEFINITIONS_EXECUTION.unlock();
}

//Should be used ONLY when a player seems to be in other server
void ServerNetworkEngine::RedirectPacket(NetworkPacket *pkt, u16 ID_) {
	//infostream << "Redirecting packets to into " << ID_ << std::endl;
	//Get peer -> player -> player_id
	if (!initialized) {
		errorstream << "Could not redirect packets, is the proxy initialized?" << std::endl;
		return;
	}
	// peer->player
	session_t peer = 0;
	peer = pkt->getPeerId();
	// player->player_id
	RemotePlayer *PLAYER = m_env->getPlayer(peer);
	u16 ID = PLAYER->player_id;
	//Real work occurs
	/*
	All of those will be translated to obtain UserID:
		TOSERVER_PLAYERPOS
		TOSERVER_GOTBLOCKS
		TOSERVER_DELETEDBLOCKS
		TOSERVER_INVENTORY_ACTION
		TOSERVER_CHAT_MESSAGE (This should be handled by main server, and send to players a response)
		TOSERVER_DAMAGE
		TOSERVER_PLAYERITEM
		TOSERVER_RESPAWN
		TOSERVER_INTERACT
		TOSERVER_REMOVED_SOUNDS
		TOSERVER_NODEMETA_FIELDS
		TOSERVER_INVENTORY_FIELDS
	*/
	
	//Increment size to handle player ID (uint16_t)
	//pkt->incrementSize(2);
	//*pkt << ID;
	
	ActiveServersStructure *SERVER__ = StoredServers[ID_];
	if (SERVER__ == nullptr) {
		errorstream << "SERVER ID " << ID_ << " ARE NOT STORED CORRECTLY!!!" << std::endl;
		throw FailureOnIndexingARegisteredServer();
	}
	
	pkt->setPlayerID(ID);
	
	if (pkt->getCommand() == 0x23) {
		//Handled by SlaveServerObject
		SERVER__->SlaveServerObject->movePlayer(pkt);
		return;
	}
	
	if (!SERVER__->SocketConn) {
		SERVER__->srv_connection->Send((session_t)1, clientCommandFactoryTable[pkt->getCommand()].channel, pkt,
(bool)clientCommandFactoryTable[pkt->getCommand()].reliable);
	} else {
		//Send raw bytes
		if (SERVER__->SocketReady) {
			//Decompile the packet manually
			std::vector<uint8_t> raw_data;
			raw_data.resize(2+pkt->getSize());
			writeU16(&raw_data[0], pkt->getCommand());
			memcpy(&raw_data[2], pkt->getU8Ptr(0), pkt->getSize()+2);
			send(SERVER__->socket_id, raw_data.data(), raw_data.size(), MSG_NOSIGNAL);
		} else {
			warningstream << FUNCTION_NAME << ": Going to send packet " << pkt->getCommand() << " on a non-open socket!" << std::endl;
			return;
		}
	}
	//delete pkt;
	
}

void ServerNetworkEngine::SendDisconnectONLY(u16 player_id, u16 ID) {
	infostream << "Sending Disconnect packets to " << ID << ":" << player_id << std::endl;
	//Get peer -> player -> player_id
	if (!initialized) {
		errorstream << "Could not redirect packets, is the proxy initialized?" << std::endl;
		return;
	}
	
	NetworkPacket pkt(0x58, 6); //2Bytes = uint16_t [0x53] 
	pkt << player_id;
	ActiveServersStructure *SERVER__ = StoredServers[ID];
	if (SERVER__ == nullptr) {
		return;
	}
	//SERVER__->srv_connection->Send((session_t)1, 1, &pkt, true);
	SendRawToSlaveServer(ID, pkt);
	
	RemotePlayer *player = m_env->getPlayer(player_id);
	
	if (!player)
		return;
	
	player->OnServer->IsApplied = false;
	player->OnServer->ID = 0;
}

// idk
void ServerNetworkEngine::SendDisconnect(u16 player_id, u16 ID) {
	infostream << "Sending Disconnect packets to " << ID << ":" << player_id << std::endl;
	//Get peer -> player -> player_id
	if (!initialized) {
		errorstream << "Could not redirect packets, is the proxy initialized?" << std::endl;
		return;
	}
	
	NetworkPacket pkt(0x58, 6); //2Bytes = uint16_t [0x53] 
	pkt << player_id;
	ActiveServersStructure *SERVER__ = StoredServers[ID];
	if (SERVER__ == nullptr) {
		return;
	}
	//SERVER__->srv_connection->Send((session_t)1, 1, &pkt, true);
	SendRawToSlaveServer(ID, pkt);
	
	RemotePlayer *player = m_env->getPlayer(player_id);
	
	player->OnServer->IsApplied = false;
	player->OnServer->ID = 0;
	
	DoReCreateClient(player->player_id);
	
	SERVER__->SSO->ClearPlayerVision(player->player_id);
}

/* PLAYER CLEARING SCENARIO */
//This deletes all huds that the player has
void ServerNetworkEngine::SendWipeAllHuds(u16 pid) {
	verbosestream << FUNCTION_NAME << std::endl;
	RemotePlayer *player = m_env->getPlayer(pid);
	u32 c = player->getMaxHuds();
	for (u32 i = 0; i < c; i++)
		Server___->hudRemove(player, i);
	
	player->clearHud();
	//Server___->Send(&pkt);
}
void ServerNetworkEngine::SendWipeChat(u16 pid) {
	RemotePlayer *player = m_env->getPlayer(pid);
	for (u8 i = 0; i < 25; i++) {
		//nothing for now....
	}
}
void ServerNetworkEngine::SendActivateBackground(u16 playeri) {
	verbosestream << FUNCTION_NAME << std::endl;
	u32 imgH, txtH = 0;
	RemotePlayer *player = m_env->getPlayer(playeri);
	HudElement *img = new HudElement;
	img->type = HUD_ELEM_IMAGE;
	img->pos = v2f(0.0f,0.0f);
	img->text = std::string("default_dirt.png");
	img->name = std::string("MineStarsLoadingBackground");
	img->scale = v2f(100.0f,100.0f);
	imgH = Server___->hudAdd(player, img);
	HudElement *txt = new HudElement;
	txt->type = HUD_ELEM_TEXT;
	txt->pos = v2f(1.0f,1.0f);
	txt->text = std::string("Please wait until we finish loading the server...");
	txt->offset = v2f(0.0f,0.0f);
	txtH = Server___->hudAdd(player, txt);
	fun_huds[playeri] = std::make_tuple(imgH, txtH);
}
void ServerNetworkEngine::SendDeactivateBackground(u16 player) {
	verbosestream << FUNCTION_NAME << std::endl;
	RemotePlayer *obj = m_env->getPlayer(player);
	u32 imgH, txtH = 0;
	std::tie(imgH, txtH) = fun_huds[player];
	Server___->hudRemove(obj, imgH);
	Server___->hudRemove(obj, txtH);
}

//Everyone that joins to other server should wait 3s (showing an background image) for deleting hud, chat text, objects and more
void ServerNetworkEngine::QueueSendConnect(u16 player, u16 srv) {
	verbosestream << FUNCTION_NAME << std::endl;
	_SNE_SC_THR_OBJ->lock(); //Locks for no race behaviour
	_SNE_SC_THR_OBJ->queue_sc(player, srv);
	_SNE_SC_THR_OBJ->unlock();
}

void ServerNetworkEngine::_HELPER_SendConnect(u16 player_id, u16 ID) {
	verbosestream << FUNCTION_NAME << std::endl;
	infostream << "Sending Connect packets to " << ID << ":" << player_id << std::endl;
	if (!initialized) {
		errorstream << "Could not redirect packets, is the proxy initialized?" << std::endl;
		return;
	}
	ActiveServersStructure *SERVER__ = StoredServers[ID];
	if (SERVER__ == nullptr) {
		errorstream << "SERVER ID " << ID << " ARE NOT STORED CORRECTLY!!! [SendConnect]" << std::endl;
		return;
	}
	RemotePlayer *player = m_env->getPlayer(player_id);
	if (!player) {
		errorstream << "Unable to get player for PID: " << player_id << ". Are player online?" << std::endl;
		return;
	}
	u8 ser_ver = Server___->getSerializationVersion(player->getPeerId());
	if (ser_ver == SER_FMT_VER_INVALID) {
		errorstream << "ServerNetworkEngine::SendConnect: Is player connected? {peer_id=" << player->getPeerId() << ",player_id=" << player_id << "}"
<< std::endl;
		return;
	}
	
	ClientInfo info;
	if (!Server___->getClientInfo(player->getPeerId(), info)) {
		warningstream << "SendConnect[PROXY]: no client info!" << std::endl;
		return;
	}
	
	NetworkPacket pkt(0x59, 2 + std::strlen(player->getName()) + 2 + 2 + 1);
	
	PlayerSAO *sao = player->getPlayerSAO();
	u16 sao_id = sao->getId();
	pkt << player_id;
	pkt << player->proto_min;
	pkt << player->proto_max;
	pkt << player->protocol_version;
	pkt << ser_ver;
	pkt << sao_id;
	pkt << std::string(player->getName());
	pkt << info.platform;
	pkt << info.vers_string;
	
	SendRawToSlaveServer(ID, pkt);
	//SERVER__->srv_connection->Send((session_t)1, (u8)0, &pkt, true);

	player->OnServer->IsApplied = true;
	player->OnServer->ID = ID;

	//Set status

	if (Server___->PeerIdPlayers.Get(player->getPeerId()) != nullptr) {
		delete Server___->PeerIdPlayers.Get(player->getPeerId());
		Server___->PeerIdPlayers.Erase(player->getPeerId());
	}
	Server___->PeerIdPlayers.Lock();

	PlayerInternalInfo *pl = new PlayerInternalInfo();
	pl->PlayerID = player->getPlayerID();
	pl->ServerID = ID;
	pl->PeerID = player->getPeerId();
	pl->PlayingOnServ = true;
	Server___->PeerIdPlayers.unLock();
	Server___->PeerIdPlayers.Set(player->getPlayerID(), pl);
	
}

// ...
void ServerNetworkEngine::SendConnect(u16 player_id, u16 ID) {
	infostream << "Sending Connect packets to " << ID << ":" << player_id << std::endl;
	if (!initialized) {
		errorstream << "Could not redirect packets, is the proxy initialized?" << std::endl;
		return;
	}
	RemotePlayer *player = m_env->getPlayer(player_id);
	if (!player) {
		errorstream << "Unable to get player for PID: " << player_id << ". Are player online?" << std::endl;
		return;
	}
	ActiveServersStructure *SERVER__ = StoredServers[ID];
	if (SERVER__ == nullptr) {
		errorstream << "SERVER ID " << ID << " ARE NOT STORED CORRECTLY!!! [SendConnect]" << std::endl;
		return;
	}
	
	SERVER__->SlaveServerObject->registerPlayerToSlave(player->getPeerId(), player->player_id);
	Server___->SendRemoveObjectsToClient(player->player_id, false);
	SendWipeAllHuds(player_id);
	SendWipeChat(player_id);
	
	//On the last second the SC_THR should remove the hud which is implemented to don't show the player the modifications
	//SendActivateBackground(player_id); //FIXEDME: Move to the queue in the sector 2200ms
	
	player->to_other_server = true;
	
	//[OPERATOR!]:OnServer{SlaveServer} TO GET operator! AT {__DEFAULT}.h<L:88>
	if (!player->OnServer->IsApplied) {
		actionstream << "Going to disconnect player " << player_id << " from main server" << std::endl;
		session_t PID = player->getPeerId();
		//ClientDeletionReason rs = CDR_LEAVE; //by default
		//Server___->OnlyDeleteSAO(PID, ID); //delete the client if stablished here
		//Server___->RunScriptsOnLeave(PID);
		hasDeletedSAO[player_id] = false;
	} else { //$REVERSE
		SendDisconnect(player_id, player->OnServer->ID); //Lua: playerdata.OnServer.ID
	}
	
	actionstream << "SendConnect queued, for player: " << std::string(player->getName()) << std::endl;
	
	PlayerSAO *sao = player->getPlayerSAO();
	u16 sao_id = sao->getId();
	PlayerSaoIDs[player_id] = sao_id;
	
	QueueSendConnect(player_id, ID);
}

//EVER 16BIT
//This will send to all slave servers a certain ID to don't use for any active object
/*void ServerNetworkEngine::SendOccupyID(u16 PSaoID) {
	for (auto it = StoredServers.begin(); it != StoredServers.end(); ++it) {
		NetworkPacket pkt(0x95, 2);
		pkt << PSaoID;
		SERVER__->srv_connection->Send((session_t)1, (u8)1, &pkt, true);
	}
}*/

bool ServerNetworkEngine::isValidServer(u16 sid) {
	auto it = StoredServers.find(sid);
	return it != StoredServers.end();
}

//Makes the player join the main proxy
void ServerNetworkEngine::DoReCreateClient(u16 playerid) {
	RemotePlayer *p = m_env->getPlayer(playerid);
	Server___->QueuePlayerToInitialize(p->getPeerId());
}

void ServerNetworkEngine::_handle_null(NetworkPacket *pkt) {
	//Nothing
}

void ServerNetworkEngine::_handle_direct(NetworkPacket *pkt) {
	// Get the server id
	uint16_t PlayerID;
	ActiveServersStructure *SERVER__;
	u8 uint8_tSrvID = 0;
	u16 SrvID = 0;
	bool empty_pkt = true;
	if (pkt->getSize() > 1) {
		*pkt >> uint8_tSrvID;
		//Convert
		SrvID = (u16)uint8_tSrvID;
		SERVER__ = StoredServers[SrvID];
		if (SERVER__ == nullptr) {
			return;
		}
		empty_pkt = false;
	} else {
		warningstream << "Got a command from slave with no info!" << std::endl;
		return;
	}
	// Transform data
	*pkt >> PlayerID;
	for (int i = 0; i < 3; i++) {
		pkt->decreaseFront();
		pkt->decreaseReadOffset(1);
	}
	pkt->updateData();
	// Send
	RemotePlayer *player = m_env->getPlayer(PlayerID);
	if (!player)
		return;
	pkt->setPeerID(player->getPeerId());
	NetworkPacket D_PKT = *pkt;
	Server___->QueueSendTo(player->getPeerId(), D_PKT, true);
}

// Functions

void ServerNetworkEngine::handleDeleteParticleSpawner(NetworkPacket *pkt) {
	GET_SRV_ID;
	std::vector<RemotePlayer *> pLST = m_env->getPlayers();
	//Make a new network packet of 4 bytes
	NetworkPacket newpakct(TOCLIENT_DELETE_PARTICLESPAWNER, 4);
	s32 data;
	*pkt >> data; //last bytes = 4
	Server___->Players.Lock();
	for (auto i = Server___->Players.GetRawMap()->begin(); i != Server___->Players.GetRawMap()->end(); i++) {
		//Filter players
		if (i->second.ServerID == SrvID) {
			Server___->QueueSendTo(i->second.PeerID, newpakct, true);
		}
	}
	Server___->Players.unLock();
}

void ServerNetworkEngine::handleAddNode(NetworkPacket *pkt) {
	GET_SRV_ID;
	std::vector<uint16_t> PlayersID;
	uint16_t PlayersTableSize;
	*pkt >> PlayersTableSize;
	if (PlayersTableSize == 0)
		return;
	for (uint16_t i = 0; i < PlayersTableSize; i++) {
		uint16_t pid = 0;
		*pkt >> pid;
		if (pid == 0) {
			errorstream << "Missed 'Extended TOCLIENT_ADDNODE' '0x64': Packet are received incorrectly" << std::endl;
			return;
		}
		PlayersID.push_back(pid);
	}
	//Get the data
	v3s16 pos;
	uint16_t param0;
	uint8_t param1;
	uint8_t param2;
	uint8_t rmtdt; //remove_metadata
	*pkt >> pos;
	*pkt >> param0;
	*pkt >> param1;
	*pkt >> param2;
	*pkt >> rmtdt;
	//Export to every player
	for (uint16_t PID : PlayersID) {
		RemotePlayer *player = m_env->getPlayer(PID);
		NetworkPacket newpakct(TOCLIENT_ADDNODE, 11);
		newpakct << pos << param0 << param1 << param2 << rmtdt;
		infostream << "Sending 11 bytes of data to peer: " << player->getPeerId() << " sent from Slave Server: " << SrvID << std::endl;
		Server___->QueueSendTo(player->getPeerId(), newpakct, true);
	}
}

void ServerNetworkEngine::handleCaseDisconnect(NetworkPacket *pkt) {
	GET_SRV_ID;
	uint16_t player;
	uint8_t signal; //1=Move, 0=Disconnect from server to main proxy server
	uint8_t toserv; //if signal is 1 then move to that server
	*pkt >> player >> signal;
	bool _;
	actionstream << "Disconnecting player: " << player << " from slave: " << SrvID << std::endl;
	if (pkt->getRemainingBytes() > 0) {
		*pkt >> toserv;
	}
	if (signal > 0) {
		//SendDisconnect(player, (u16)SrvID); //handled by SendConnect
		SendConnect(player, (uint16_t)toserv);
	} else {
		RemotePlayer *p = m_env->getPlayer(player);
		if (!p)
			return;
		m_env->loadPlayer(p, &_, p->getPeerId(), false);
		DoReCreateClient(player); //may queue to server
	}
}

void ServerNetworkEngine::handleHello(NetworkPacket *pkt) {
	GET_SRV_ID;
	sendQueryItemData(SrvID);
}

void ServerNetworkEngine::handleRemoveNode(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Got TOCLIENT_REMOVENODE" << std::endl;
	std::vector<uint16_t> PlayersID;
	v3s16 pos;
	uint16_t pc;
	*pkt >> pos >> pc;
	for (uint16_t i = 0; i < pc; i++) {
		uint16_t p = 0;
		*pkt >> p;
		PlayersID.push_back(p);
	}
	NetworkPacket newpakct(TOCLIENT_REMOVENODE, 6);
	newpakct << pos;
	for (uint16_t PID : PlayersID) {
		RemotePlayer *player = m_env->getPlayer(PID);
		Server___->QueueSendTo(player->getPeerId(), newpakct, true);
	}
}

//This function acts like a handshake when a player joins the other server
void ServerNetworkEngine::handleRemoveSAO(NetworkPacket *pkt) {
	GET_SRV_ID;
	actionstream << "Deleting SAO of player a player, sent by a slave: " << SrvID << std::endl;
	uint16_t playerid = 0;
	*pkt >> playerid;
	SERVER__->players_protonet[playerid] = m_env->getPlayer(playerid)->protocol_version > 37;
	verbosestream << FUNCTION_NAME << ": to delete:" << playerid << std::endl;
	if (!hasDeletedSAO[playerid]) {
		RemotePlayer *p = m_env->getPlayer(playerid);
		if (p == NULL) {
			errorstream << "Could not acquire player object from player id: " << playerid << std::endl;
			return;
		}
		Server___->OnlyDeleteSAO(p->getPeerId(), SrvID);
	}
}

void ServerNetworkEngine::handleUpdateList(NetworkPacket *pkt) {
	GET_SRV_ID;
	SERVER__->SlaveServerObject->updatePlayerSaoList(pkt);
}

void ServerNetworkEngine::handleGotItemDef(NetworkPacket *pkt) {
	GET_SRV_ID;
	//Get ISTREAM
	actionstream << "Received ItemDefinitions" << std::endl;
	std::istringstream tmp_is(pkt->readLongString(), std::ios::binary);
	std::ostringstream tmp_os;
	decompressZlib(tmp_is, tmp_os);
	std::istringstream tmp_is2(tmp_os.str());

	Slave_ItemDef->itemDefReceived(SrvID, tmp_is2);

	actionstream << "Received NodeDefinitions" << std::endl;
	std::istringstream tmp_is21(pkt->readLongString(), std::ios::binary);
	std::ostringstream tmp_os2;
	decompressZlib(tmp_is21, tmp_os2);
	std::istringstream tmp_is2_(tmp_os2.str());

	Slave_ItemDef->nodeDefReceived(SrvID, tmp_is2_);

	//Start
	Slave_ItemDef->startTransformation(SrvID);

	//Send 'WE REQUIRE THE DATA'
	NetworkPacket pky(0x116, 0);
	//SERVER__->srv_connection->Send((session_t)1, (u8)0, &pky, true);
	SendRawToSlaveServer(SrvID, pky);
}

void ServerNetworkEngine::handleOnReceiveAOM(NetworkPacket *pkt) {
	GET_SRV_ID;
	verbosestream << "MSG 0x73: Received." << std::endl;
	//Queue thing
	SERVER__->mutex.lock();
	SERVER__->aomMsgs.push_back(*pkt);
	SERVER__->mutex.unlock();
}

void ServerNetworkEngine::handleGotMedia(NetworkPacket *pkt) {
	GET_SRV_ID;
	bool not_empty = loadMediaNcheckSums(SrvID, pkt);
	if (not_empty) {
		NetworkPacket pkt2 = sendGetMedia(SrvID);
		SendRawToSlaveServer(SrvID, pkt2);
	}
}

void ServerNetworkEngine::handleGotMedia_STAGETWO(NetworkPacket *pkt) {
	GET_SRV_ID;
	startLoadSlaveMedia(SrvID, pkt);
}

void ServerNetworkEngine::handleHandShake_STAGETWO(NetworkPacket *pkt) {
	GET_SRV_ID;
	//Register the player into the SSO object
	uint16_t player_id, player_sao_id_on_slave, real_sao;
	*pkt >> player_id >> player_sao_id_on_slave;
	try {
		real_sao = PlayerSaoIDs.at(player_id);
	} catch (std::exception &e) {
		//Do nothing
	}
	SERVER__->SSO->RegisterPlayer(player_id, real_sao, player_sao_id_on_slave);
}

void ServerNetworkEngine::handlePlayerInitialization(NetworkPacket *pkt) {
	GET_SRV_ID;
	verbosestream << "MSG 0x77: Received." << std::endl;
	std::string initialization_data;
	u16 _player;
	*pkt >> _player;
	initialization_data = std::string(pkt->getString(3), pkt->getSize()-3); //2:player, 1:srv_id, 4:size(UNUSED)
	//For some reason, the server crashes, so i need to see where and from what are crashing, so i will store a file in MineStars/ folder for the
	// raw data
	SAVEFILE("/NUT.txt", initialization_data);

	//Needed for Message scraping
	RemotePlayer *player = m_env->getPlayer(_player);
	bool compat = false;
	if (player) {
		compat = player->protocol_version < 37;
	}
	//Prepare everything to send, now we will transform the packet
	std::istringstream stream(initialization_data, std::ios_base::binary);
	v3f pos = readV3F(stream, player->protocol_version); //Y
	v3f rot = readV3F32(stream); //Y
	f32 pit = readF32(stream); //Y
	u16 hp = readU16(stream); //Y [Server -> SendHP]
	//Messages
	u8 count = readU8(stream); //We know about those 4 first messages
	std::string prop = deSerializeString32(stream);
	std::string armor = deSerializeString32(stream);
	std::string anim = deSerializeString32(stream);
	std::string attach = deSerializeString32(stream);
	std::deque<std::string> other_msgs;
	for (uint8_t i = 0; i < count; i++) {
		std::string msg = deSerializeString32(stream);
		other_msgs.push_back(msg);
	}
	//Prepare to send the data into the player
	//NOTE: This modifications are sent only TO A PLAYER
	//Send those first modfifications
	std::string to;
	u16 playerSAO_ID = PlayerSaoIDs.at(_player);
	//Send in the right format
	//FIXME: Fix the packet receiver in removeaddOBJECTS() to only send packets of initialization to other people rather the own player, the
	// own player maybe will receive in AOM mode
	char buf[2];
	u16 msgs = 4;
	writeU16((u8*)buf, playerSAO_ID);
	NetworkPacket _1_pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES, 0);

	//Properties
	to.append(buf, 2);
	to.append(serializeString16(prop));
	//Armor
	to.append(buf, 2);
	to.append(serializeString16(armor));
	//Animations (proto req)
	to.append(buf, 2);
	to.append(serializeString16(anim));
	//Attach
	to.append(buf, 2);
	to.append(serializeString16(attach));

	//SendMovePlayer
	NetworkPacket _movepkt(TOCLIENT_MOVE_PLAYER, sizeof(v3f) + sizeof(f32) * 2);
	_movepkt << pos << pit << rot.Y;

	//SendHP
	if (player->protocol_version && player->protocol_version >= 37) {
		NetworkPacket npkt(TOCLIENT_HP, 2, player->getPeerId());
		npkt << hp;
		Server___->QueueSendTo(player->getPeerId(), npkt, true);
	} else {
		NetworkPacket npkt(TOCLIENT_HP, 1, player->getPeerId());
		u8 rhp = hp & 0xFF;
		npkt << rhp;
		Server___->QueueSendTo(player->getPeerId(), npkt, true);
	}

	//Other msgs
	while (!other_msgs.empty()) {
		to.append(buf, 2);
		to.append(serializeString16(other_msgs.front()));
		other_msgs.pop_front();
		msgs++;
	}

	//Prepare
	_1_pkt.putRawString(to.c_str(), to.size());

	//Send
	verbosestream << FUNCTION_NAME << ": 0x77: Sending back pkts" << std::endl;
	Server___->QueueSendTo(player->getPeerId(), _1_pkt, true);
	Server___->QueueSendTo(player->getPeerId(), _movepkt, true);
}

void ServerNetworkEngine::handleTimeOfDay(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_TIME_OF_DAY: " << SrvID << std::endl;
	u16 time;
	f32 speed;

	*pkt >> time;
	*pkt >> speed;

	NetworkPacket Npkt(TOCLIENT_TIME_OF_DAY, 0, 0, 37);
	NetworkPacket Nlegacypkt(TOCLIENT_TIME_OF_DAY, 0, 0, 32);

	Npkt << time << speed;
	Nlegacypkt << time << speed;

	Server___->DoSendToEveryone(&Npkt, &Nlegacypkt, SrvID);
}

void ServerNetworkEngine::handleChatMessage(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_CHAT_MESSAGE: " << SrvID << std::endl;
	//Initialize
	u8 version, message_type = 0;
	bool mode = false;
	u16 PlayerID = 0;
	std::wstring message = L"";
	std::wstring sender = L"";
	u64 timestamp;
	//Store
	*pkt >> PlayerID;
	*pkt >> mode;
	*pkt >> version;
	*pkt >> message_type;
	*pkt >> sender;
	*pkt >> message;
	*pkt >> timestamp;

	if (mode) { //mode==0
		RemotePlayer *player = m_env->getPlayer(PlayerID);
		if (!player)
			return;
		if (player->protocol_version < 35) {
			NetworkPacket legacypkt(TOCLIENT_CHAT_MESSAGE_OLD, 0);
			legacypkt << message;
			Server___->QueueSendTo(player->getPeerId(), legacypkt, true);
		} else {
			NetworkPacket Npkt(TOCLIENT_CHAT_MESSAGE, 0);
			Npkt << version << message_type << sender << message << timestamp;
			Server___->QueueSendTo(player->getPeerId(), Npkt, true);
		}
		return;
	} else {
		//New version
		NetworkPacket Npkt(TOCLIENT_CHAT_MESSAGE, 0);
		Npkt << version << message_type << sender << message << timestamp;
		//Old version
		NetworkPacket legacypkt(TOCLIENT_CHAT_MESSAGE_OLD, 0);
		legacypkt << message;
		//Send
		Server___->DoSendToEveryone(&Npkt, &legacypkt, 35);
		return;
	}
}

void ServerNetworkEngine::handleHP(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_HP: " << SrvID << std::endl;
	u16 pid, hp = 0;
	*pkt >> pid;
	*pkt >> hp;
	RemotePlayer *player = m_env->getPlayer(pid);
	if (player) {
		u16 proto = player->protocol_version;
		if (proto && proto >= 37) {
			NetworkPacket npkt(TOCLIENT_HP, 2, player->getPeerId());
			npkt << hp;
			Server___->QueueSendTo(player->getPeerId(), npkt, true);
			return;
		} else {
			NetworkPacket npkt(TOCLIENT_HP, 1, player->getPeerId());
			u8 rhp = hp & 0xFF;
			npkt << rhp;
			Server___->QueueSendTo(player->getPeerId(), npkt, true);
			return;
		}
	}
}

void ServerNetworkEngine::handlePlaySound(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_PLAY_SOUND: " << SrvID << std::endl;
	u16 pcount = 0;
	*pkt >> pcount;
	std::vector<u16> plist;
	for (u16 i = 0; i < pcount; i++) {
		u16 pid = 0;
		*pkt >> pid;
		verbosestream << "TOCLIENT_PLAY_SOUND: Player added: " << pid << std::endl;
		plist.push_back(pid);
	}
	//some data of audio
	s32 ID;
	std::string NAME;
	float GAIN, PITCH, FADE;
	u8 TYPE;
	v3f POSI;
	u16 OBJE;
	bool LOOP, EPHEMERAL;
	//initliaze
	*pkt >> ID;
	*pkt >> NAME;
	*pkt >> GAIN;
	*pkt >> TYPE;
	*pkt >> POSI;
	*pkt >> OBJE;
	*pkt >> LOOP;
	*pkt >> FADE;
	*pkt >> PITCH;
	*pkt >> EPHEMERAL;
	//send
	NetworkPacket Npkt(TOCLIENT_PLAY_SOUND, 0);
	NetworkPacket legacypkt(TOCLIENT_PLAY_SOUND, 0, PEER_ID_INEXISTENT, 32);
	Npkt << ID << NAME << GAIN << TYPE << POSI << OBJE << LOOP << FADE << PITCH << EPHEMERAL;
	legacypkt << ID << NAME << GAIN << TYPE << POSI << OBJE << LOOP << FADE;
	for (u16 ID : plist) {
		RemotePlayer *player = m_env->getPlayer(ID);
		u16 proto_ver = player->protocol_version;
		if ((GAIN > 0) && proto_ver < 32)
			continue;
		if (proto_ver >= 37) {
			Server___->QueueSendTo(player->getPeerId(), Npkt, true);
		} else {
			//Legacy send
			Server___->QueueSendTo(player->getPeerId(), legacypkt, true);
		}
	}
}

void ServerNetworkEngine::handleStopSound(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_STOP_SOUND: " << SrvID << std::endl;
	s32 ID_;
	u16 PCOUNT;
	std::vector<u16> PLIST;
	*pkt >> ID_;
	*pkt >> PCOUNT;
	for (u16 i = 0; i < PCOUNT; i++) {
		u16 pid = 0;
		*pkt >> pid;
		PLIST.push_back(pid);
	}
	NetworkPacket NPKT(TOCLIENT_STOP_SOUND, 4);
	NPKT << ID_;
	for (u16 ID : PLIST) {
		RemotePlayer *player = m_env->getPlayer(ID);
		Server___->QueueSendTo(player->getPeerId(), NPKT, true); //RELIABLE=TRUE
	}
}

void ServerNetworkEngine::handleFadeSound(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_FADE_SOUND: " << SrvID << std::endl;
	s32 HANDLE;
	float STEP, GAIN;
	u16 SIZE;
	///////////
	*pkt >> SIZE;
	std::vector<u16> PLIST;
	for (u16 i = 0; i < SIZE; i++) {
		u16 cc;
		*pkt >> cc;
		PLIST.push_back(cc);
	}
	*pkt >> HANDLE >> STEP >> GAIN;

	NetworkPacket Npkt(TOCLIENT_FADE_SOUND, 4);
	Npkt << HANDLE << STEP << GAIN;

	NetworkPacket compat_pkt(TOCLIENT_STOP_SOUND, 4);
	compat_pkt << HANDLE;

	for (u16 ID : PLIST) {
		RemotePlayer *p = m_env->getPlayer(ID);
		if (p->protocol_version >= 32) {
			Server___->QueueSendTo(p->getPeerId(), Npkt, true);
		} else {
			Server___->QueueSendTo(p->getPeerId(), compat_pkt, true);
		}
	}
}

void ServerNetworkEngine::handleDetachedInventory(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_DETACHED_INVENTORY: " << SrvID << std::endl;
	//idk to say in this glorious comment
	u16 p_id = 0;
	*pkt >> p_id;
	std::string name;
	*pkt >> name;
	bool kpt = true;
	*pkt >> kpt;

	u16 size = 0;
	if (pkt)
		*pkt >> size;

	std::string contents(pkt->getRemainingString(), pkt->getRemainingBytes());
	if (p_id == (u16)0) { //Means to send to everyone in the same server
		//std::vector<RemotePlayer *> pLST = m_env->getPlayers();
		NetworkPacket newpakct(TOCLIENT_DETACHED_INVENTORY, 0);
		NetworkPacket legacy_pkt(TOCLIENT_DETACHED_INVENTORY, 0);
		newpakct << name << kpt << size;
		legacy_pkt << name;
		newpakct.putRawString(contents);
		legacy_pkt.putRawString(contents);
		Server___->Players.Lock();
		for (auto i = Server___->Players.GetRawMap()->begin(); i != Server___->Players.GetRawMap()->end(); i++) {
			//Filter players
			if (i->second.ServerID == SrvID) {
				if (m_env->getPlayer(i->second.PlayerID)->protocol_version >= 37)
					Server___->QueueSendTo(i->second.PeerID, newpakct, true);
					else
						Server___->QueueSendTo(i->second.PeerID, legacy_pkt, true);
			}
		}
		Server___->Players.unLock();

	} else {
		RemotePlayer *p = m_env->getPlayer(p_id);
		if (p) {
			NetworkPacket newpakct(TOCLIENT_DETACHED_INVENTORY, 0);
			NetworkPacket legacy_pkt(TOCLIENT_DETACHED_INVENTORY, 0);
			newpakct << name << kpt << size;
			legacy_pkt << name;
			newpakct.putRawString(contents);
			legacy_pkt.putRawString(contents);
			if (p->protocol_version >= 37)
				Server___->QueueSendTo(p->get_peer_id(), newpakct, true);
				else
					Server___->QueueSendTo(p->get_peer_id(), legacy_pkt, true);
		}
	}
}

void ServerNetworkEngine::handleNodemetaChanged(NetworkPacket *pkt) {
	GET_SRV_ID;
	infostream << "Processing Slave-Sent command TOCLIENT_NODEMETA_CHANGED: " << SrvID << std::endl;
	//std::vector<RemotePlayer *> pLST = m_env->getPlayers();
	Server___->Players.Lock();
	for (auto i = Server___->Players.GetRawMap()->begin(); i != Server___->Players.GetRawMap()->end(); i++) {
		//Filter players
		if (i->second.ServerID == SrvID) {
			NetworkPacket D_PKT = *pkt;
			pkt->setPeerID(i->second.PeerID);
			Server___->QueueSendTo(i->second.PeerID, D_PKT, true);
		}
	}
	Server___->Players.unLock();
}

//NOTE: Obsolete
/*
void ServerNetworkEngine::handle_AOM(NetworkPacket *pkt) {
	infostream << "Processing Slave-Sent command TOCLIENT_ACTIVE_OBJECT_MESSAGES: " << SrvID << std::endl;
	u8 reliable = 0;
	*pkt >> PlayerID;
	*pkt >> reliable;
	//input-stream to output-stream
	std::string datastring(pkt->getString(4), pkt->getSize()-4);
	NetworkPacket newpkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES, 0);
	newpkt.putRawString(datastring.c_str(), datastring.size());

	pkt->setChannel(reliable ? 0 : 1);
	if (reliable == 1) { //Send to channel 0 as reliable
		verbosestream << "Sending TOCLIENT_ACTIVE_OBJECT_MESSAGES to " << PlayerID << "reliable=true" << std::endl;
		RemotePlayer *p = m_env->getPlayer(maskedu16(PlayerID));
		Server___->QueueSendTo(p->getPeerId(), newpkt, true);
	} else { //Sento to channel 1 as unreliable
		verbosestream << "Sending TOCLIENT_ACTIVE_OBJECT_MESSAGES to " << PlayerID << "reliable=false" << std::endl;
		RemotePlayer *p = m_env->getPlayer(maskedu16(PlayerID));
		Server___->QueueSendTo(p->getPeerId(), newpkt, false);
	}
}*/

void ServerNetworkEngine::handleAO_RA(NetworkPacket *pkt) {
	GET_SRV_ID;
	uint16_t PlayerID;
	*pkt >> PlayerID;

	if (SERVER__->SSO == nullptr)
		return;

	SlaveServerObjects *SSO = SERVER__->SSO;

	//New system
	NetworkPacket n_pkt(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD, 0);
	u8 type;
	u16 removed_count, added_count, id;
	char buf[4];
	std::string rawdata;

	*pkt >> removed_count;

	writeU16((u8*)buf, removed_count);
	rawdata.append(buf, 2);

	for (u16 i = 0; i < removed_count; i++) {
		*pkt >> id;

		//Get ID from registering so we can remove safely the id with an fake ud
		u16 fake_id = SSO->GetObjectByPlayer(PlayerID, id);
		if (fake_id == NOT_KNOWN_PLAYER_ID || fake_id == UNKNOWN_OBJECT_ID || fake_id == PLAYER_OBJECT_ID)
			continue;

		writeU16((u8*)buf, fake_id);
		rawdata.append(buf, 2);
	}

	*pkt >> added_count;
	writeU16((u8*)buf, added_count);
	rawdata.append(buf, 2);

	for (u16 i = 0; i < added_count; i++) {
		*pkt >> id >> type;

		//The target player should not receive the initialization data of himself.
		//So, i will redirect the initialization data to AOM.
		//Let's get deeper

		//We found that the player ins't initialized, lets call other functions to handle this
		if (SSO->EqualsSAOidWITHgivenRes(PlayerID, id)) { //Check if provided SAO are the SAO which the slave send to us
			// If true, then the provided initialization data are initialization data for PlayerID
			// Send a request for that raw data, allowing 0x102
			NetworkPacket pkt(0x101, 2);
			pkt << id << PlayerID;
			SendRawToSlaveServer(SrvID, pkt);
			//true;
			continue;
		}

		//Register object in the seeing of the player
		u16 fks_id = SSO->RegisterObjectForPlayer(PlayerID, id);

		writeU16((u8*)buf, fks_id);
		rawdata.append(buf, 2);
		writeU8((u8*)buf, type);
		rawdata.append(buf, 1);

		verbosestream << "Going to send register object to player '" << PlayerID << "', RealObjId=" << id << ", FakeId=" << fks_id <<
		std::endl;

		//May we need to edit the long string so we can make it fit the selected id?
		//YES
		std::string stream_raw(pkt->readLongString());
		stream_raw[5] = (fks_id >> 8) & 0xFF;
		stream_raw[6] = (fks_id >> 0) & 0xFF;

		rawdata.append(serializeString32(std::string(stream_raw)));
	}

	n_pkt.putRawString(rawdata.c_str(), rawdata.size());

	RemotePlayer *player = m_env->getPlayer(PlayerID);
	Server___->QueueSendTo(player->getPeerId(), n_pkt, true);
}

const static LoaderFunctions direct = { &ServerNetworkEngine::_handle_direct };
const static LoaderFunctions null = { &ServerNetworkEngine::_handle_null };
//FUNCTIONS:: _Functions
//NOTE: The proxy always acts like a player in the slave server
const LoaderFunctions _Functions[MAX_FUNC_SCALE] = {
	//First we start from the named ones, and then the raw number ones
	null, //0x00
	null, //0x01
	null, //0x02
	null, //0x03
	null, //0x04
	null, //0x05
	null, //0x06
	null, //0x07
	null, //0x08
	null, //0x09
	null, //0x10
	null, //0x11
	null, //0x12
	null, //0x13
	null, //0x14
	null, //0x15
	null, //0x16
	null, //0x17
	null, //0x18
	null, //0x19
	direct, //0x20 TOCLIENT_BLOCKDATA
	direct, //0x21 TOCLIENT_ADDNODE [Moved to 0x64]
	direct, //0x22 TOCLIENT_REMOVENODE
	null, //0x23
	null, //0x24
	null, //0x25
	null, //0x26
	direct, //0x27 TOCLIENT_INVENTORY
	null, //0x28
	{ &ServerNetworkEngine::handleTimeOfDay }, //0x29 TOCLIENT_TIME_OF_DAY
	null, //0x2A
	direct, //0x2B TOCLIENT_PLAYER_SPEED
	null, //0x2C
	null, //0x2D
	null, //0x2E
	{ &ServerNetworkEngine::handleChatMessage }, //0x2F TOCLIENT_CHAT_MESSAGE
	null, //0x30
	{ &ServerNetworkEngine::handleAO_RA }, //0x31 TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD
	null, //0x32 TOCLIENT_ACTIVE_OBJECT_MESSAGES NOTE: Should be null as the server has a new proto to send
	{ &ServerNetworkEngine::handleHP }, //0x33 TOCLIENT_HP
	direct, //0x34 TOCLIENT_MOVE_PLAYER
	null, //0x35
	direct, //0x36 TOCLIENT_FOV
	direct, //0x37 TOCLIENT_DEATHSCREEN
	null, //0x38
	null, //0x39
	null, //0x3A
	null, //0x3B
	null, //0x3C
	null, //0x3D
	null, //0x3E
	{ &ServerNetworkEngine::handlePlaySound }, //0x3F TOCLIENT_PLAY_SOUND
	{ &ServerNetworkEngine::handleStopSound}, //0x40 TOCLIENT_STOP_SOUND
	direct, //0x41 TOCLIENT_PRIVILEGES
	direct, //0x42 TOCLIENT_INVENTORY_FORMSPEC
	{ &ServerNetworkEngine::handleDetachedInventory }, //0x43 TOCLIENT_DETACHED_INVENTORY
	direct, //0x44 TOCLIENT_SHOW_FORMSPEC
	direct, //0x45 TOCLIENT_MOVEMENT
	direct, //0x46 TOCLIENT_SPAWN_PARTICLE
	direct, //0x47 TOCLIENT_ADD_PARTICLESPAWNER
	null, //0x48
	direct, //0x49 TOCLIENT_HUDADD
	direct, //0x4A TOCLIENT_HUDRM
	direct, //0x4B TOCLIENT_HUDCHANGE
	direct, //0x4C TOCLIENT_HUD_SET_FLAGS
	direct, //0x4D TOCLIENT_HUD_SET_PARAM
	direct, //0x4E TOCLIENT_BREATH
	direct, //0x4F TOCLIENT_SET_SKY
	direct, //0x50 TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO
	direct, //0x51 TOCLIENT_LOCAL_PLAYER_ANIMATIONS
	direct, //0x52 TOCLIENT_EYE_OFFSET
	direct, //0x53 TOCLIENT_DELETE_PARTICLESPAWNER
	direct, //0x54 TOCLIENT_CLOUD_PARAMS
	{ &ServerNetworkEngine::handleFadeSound }, //0x55 TOCLIENT_FADE_SOUND
	null,
	null, //0x57
	null, //0x58
	{ &ServerNetworkEngine::handleNodemetaChanged }, //0x59 TOCLIENT_NODEMETA_CHANGED
	direct, //0x5A TOCLIENT_SET_SUN
	direct, //0x5B TOCLIENT_SET_MOON
	direct, //0x5C TOCLIENT_SET_STARS
	null, //0x5D
	null, //0x5E
	null, //0x5F
	null, //0x60
	null, //0x61
	direct, //0x62 TOCLIENT_MINIMAP_MODES [MS v4.0]
	{ &ServerNetworkEngine::handleDeleteParticleSpawner }, //0x63
	{ &ServerNetworkEngine::handleAddNode }, //0x64
	{ &ServerNetworkEngine::handleCaseDisconnect }, //0x65
	{ null },
	{ &ServerNetworkEngine::handleRemoveNode }, //0x67
	{ &ServerNetworkEngine::handleRemoveSAO }, //0x68
	{ &ServerNetworkEngine::handleUpdateList }, //0x69
	null, //0x6A  Mmmmhhmmh.... I should add features within this unused cmdid
	null, //0x6B
	null, //0x6C
	null, //0x6D
	null, //0x6E
	null, //0x6F
	{ &ServerNetworkEngine::handleGotItemDef }, //0x70
	null, //0x71
	null, //0x72
	{ &ServerNetworkEngine::handleOnReceiveAOM }, //0x73
	{ &ServerNetworkEngine::handleGotMedia }, //0x74
	{ &ServerNetworkEngine::handleGotMedia_STAGETWO }, //0x75
	{ &ServerNetworkEngine::handleHandShake_STAGETWO }, //0x76
	{ &ServerNetworkEngine::handlePlayerInitialization }, //0x77
};


// This sucks.....

//Converts all updates of the world (In player environment) of a slave server into main server (proxy) to an player (specified by the server)
//Should be called to handle actions.
//THIS MAAAAYBE SHOULD RUN ON A DIFFERENT THREAD TO HANDLE CORRECTLY PLAYER ACTIONS
void ServerNetworkEngine::DoApplyEnvironmentToPlayer(NetworkPacket *pkt) {
	verbosestream << "Receiving Environment Updates [0x" << std::hex << pkt->getCommand() << std::dec << "]" << std::endl;
	if (!initialized) {
		errorstream << "Could not redirect packets, is the proxy initialized?" << std::endl;
		return;
	}
	//The first 2 bytes is the uint16_t (ID of player)
	//The packets are overrided by networkpackets.cpp to make
	
	/*
		How this works?
		The slave servers does put a uint16_t in the first index of a command data, to recognize to where to go
		
		################
		# Slave Server #
		################
		  |              
		  |     ###############################################################
		  #----># Proxy Server [Get from u16-ID an peerID to send the packet] #
		        ###############################################################
		                       |
		                       |      #################
		                       #-----># PeerID Player #
		                              #################
	*/
	//Should load the specific function
	if (pkt->getCommand() > MAX_FUNC_SCALE) {
		errorstream <<FUNCTION_NAME << ": Unknown command: " << pkt->getCommand() << "; May the versions are not correct" << std::endl;
		return;
	}
	const LoaderFunctions &handle = _Functions[pkt->getCommand()];
	(this->*handle.handler)(pkt);
}

/* ActiveServersStructure */
ActiveServersStructure::ActiveServersStructure(ServerNetworkEngine *SNE): SNE(SNE), active(false) {
	ServerFrontStructure *ServerStruct_ = new ServerFrontStructure();
	ServerStruct = ServerStruct_;
	actionstream << "ServerStruct*" << std::endl;
}

// Registers an slave server (Func on LUA)
u16 ServerNetworkEngine::RegisterProxyServer(Address address, u16 ID_FOR_SERVER, bool IndependientBindAddress, u16 PortBindAddress) {
	ActiveServersStructure *STRUCT = new ActiveServersStructure(this);
	STRUCT->ServerStruct->address = address; //define address

	if (address == Address()) {
		//A manual socket will be added!.
		IndependientBindAddress = false; //In fact this must be true but it will be an socket, so don't specify connection address and port
		PortBindAddress = 0;
	}

	//get id
	/*size_t s___ = StoredServers.size();
	u16 s_ = (u16)s___;
	++s_;*/
	//^^ UNSUPPORTED FOR NOW
	STRUCT->ID = ID_FOR_SERVER;
	u16 s_ = ID_FOR_SERVER;
	
	//only servers to send, receive maybe
	if (PortBindAddress != 0) {
		PeerHandler__ *ph = new PeerHandler__();
		con::Connection *srv_new = new con::Connection(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, address.isIPv6(), ph);
		STRUCT->srv_connection = srv_new;
	}
	
	if (IndependientBindAddress) {
		PeerHandler__ *ph_ = new PeerHandler__();
		con::Connection *srv_new_ = new con::Connection(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, address.isIPv6(), ph_);
		STRUCT->BINDsrv_connection = srv_new_;
		Address ba(0, 0, 0, 0, PortBindAddress);
		STRUCT->ServerStruct->bind_address = ba;
		actionstream << "Server: " << s_ << " has independient address for receiving info" << std::endl;
	}
	
	STRUCT->independient = IndependientBindAddress;
	
	ActiveSSThread *thr = new ActiveSSThread(STRUCT);
	STRUCT->thread = thr;
	
	StoredServers[s_] = STRUCT;
	
	//Initialize Queue structure
	PacketsQueue[s_] = new QueuedPacketsHelper();
	actionstream << "Registered new slave server: " << address.serializeString() << std::endl;
	return s_;
}

void ServerNetworkEngine::sendQueryItemData(u16 ID) {
	NetworkPacket pkt(0x99, 0); //Means acquire
	ActiveServersStructure *ActiveSS = StoredServers[ID]; //no
	//ActiveSS->srv_connection->Send((session_t)1, (u8)0, &pkt, false);
	SendRawToSlaveServer(ID, pkt);
}

/* Protected 
//Should be run in network thread (SlaveNetThr)
//Not need for mutex as this will be MODIFIED ONLY on 1 thread
void ServerNetworkEngine::ON_STEP() {
	while (!f_q.empty()) {
		std::function<void()> fn = f_q.front();
		fn();
		f_q.pop_front();
	}
}

void ServerNetworkEngine::F_QUEUE(const std::function<void()> &f) {
	f_q.push_back(f);
}*/

void ServerNetworkEngine::CreateNewThreadStructureForServer(ActiveServersStructure *struct_) {
	std::string nameforthread = "SlaveNetThr";
	ServerNetworkEngine_NetworkThread *thread = new ServerNetworkEngine_NetworkThread(this, struct_->ID, nameforthread, struct_);
	threads.push_back(thread);
	thread->start();
}

/*
	NetworkPacket new_pkt(0x67, 0);
	it->second->srv_connection->Send((session_t)1, (u8)1, &new_pkt, true);
	[Proxy] -> Slave
	On address serve
*/

//This should send to all registered slave servers to init
void ServerNetworkEngine::OnInitServer() {
	initialized = true;
	if (!IsThisAProxy) {
		return;
	}
	//First, Load all data about server
	
	loadMeta(StoredServers.size());
	
	//Then, enable connections
	
	for (auto it = StoredServers.begin(); it != StoredServers.end(); ++it) {
		if (it->second->independient) {
			it->second->BINDsrv_connection->SetTimeoutMs(5);
			it->second->BINDsrv_connection->Serve(it->second->ServerStruct->bind_address); //Listen to address
			it->second->srv_connection->SetTimeoutMs(0);
			it->second->srv_connection->Connect(it->second->ServerStruct->address); //Connect to the slave (to address)
		} else {
			//Bind address will be the main address/port of the server (proxy)
			it->second->srv_connection->SetTimeoutMs(0);
			it->second->srv_connection->Connect(it->second->ServerStruct->address); //Connect to the slave (to address) to send info
		}
		
		//Helper for less lag
		SlaveServerPM *slave_obj1 = new SlaveServerPM(this, it->first);
		it->second->SlaveServerObject = slave_obj1;
		
		//Helper for objects manage
		SlaveServerObjects *_SSO = new SlaveServerObjects();
		it->second->SSO = _SSO;
		it->second->thread->SSO = _SSO;
		
		//Helper for blocks & items variations
		Slave_ItemDef->registerServerId(it->first);
		
		//Start thread
		CreateNewThreadStructureForServer(it->second);
		it->second->SlaveServerObject->SSO = _SSO;
		actionstream << "Initialized server for ID: " << it->first << std::endl;
		//Send 'hello' to slave server with the ID (u16)
		
		it->second->thread->start();
		
		//
		
		//we should wait for the 'hi' and then send its own private ID
	}
	
	//Start some useful threads for managing the SendConnect events
	_SNE_SC_THR_OBJ = new _SNE_SC_THR_CLASS(this);
	_SNE_SC_THR_OBJ->start(); //Start thread
	
	setMMMS(StoredServers.size());
}

void ServerNetworkEngine::InitializeServer(u16 ID) {
	loadMeta(StoredServers.size());
	SlaveServerPM *sobj = new SlaveServerPM(this, ID);
	SlaveServerObjects *_SSO = new SlaveServerObjects();
	CreateNewThreadStructureForServer(StoredServers[ID]);
	StoredServers[ID]->SlaveServerObject = sobj;
	StoredServers[ID]->SSO = _SSO;
	StoredServers[ID]->thread->SSO = _SSO;
	StoredServers[ID]->SlaveServerObject->SSO = _SSO;
	StoredServers[ID]->thread->start();
	StoredServers[ID]->active = false;
	actionstream << "Initialized server for ID: " << ID << std::endl;
}

void ServerNetworkEngine::SendRawToSlaveServer(u16 ID, NetworkPacket pkt) {
	ActiveServersStructure *SERVER__ = StoredServers[ID];
	
	if (pkt.getSize() == 0)
		return;
	
	try {
		if (SERVER__ == nullptr) {
			errorstream << "SERVER ID " << ID << " ARE NOT STORED CORRECTLY!!!" << std::endl;
			throw FailureOnIndexingARegisteredServer();
		}
	} catch (std::exception &e) {
		//null
	}
	
	if (!SERVER__->SocketConn) {
		SERVER__->srv_connection->Send((session_t)1, 0, &pkt, true);
	} else {
		//Decompile the packet manually
		std::vector<uint8_t> raw_data;
		raw_data.resize(2+pkt.getSize());
		writeU16(&raw_data[0], pkt.getCommand());
		memcpy(&raw_data[2], pkt.getU8Ptr(0), pkt.getSize()+2);
		send(SERVER__->socket_id, raw_data.data(), raw_data.size(), MSG_NOSIGNAL);
		actionstream <<FUNCTION_NAME << ": Sending packet <" << pkt.getCommand() << "> size: " << pkt.getSize() << std::endl;
	}
}

void ServerNetworkEngine::PoweroffServers() {
	NetworkPacket pkt(0x140, 1);
	pkt << (uint8_t)1;
	for (auto it = StoredServers.begin(); it != StoredServers.end(); it++) {
		SendRawToSlaveServer(it->first, pkt);
	}
}

void ServerNetworkEngine::SetThisServerAsAproxy(bool b) {
	IsThisAProxy = b;
	AreSlave = (b ? false : true);
}

void ServerNetworkEngine::StopNkillThreads() {
	for (ServerNetworkEngine_NetworkThread *thr : threads) {
		thr->stop();
		delete thr;
	}
}

bool ServerNetworkEngine::ServerHasMedia(const std::string &file) {
	return Server___->m_media.find(file) != Server___->m_media.end();
}

/* QueuedPacketsHelper */

QueuedPacketsHelper::QueuedPacketsHelper() {
	//may initialize something?
}

NetworkPacket *QueuedPacketsHelper::getPacket() {
	doLock();
	if (packets.empty()) {
		doUnLock();
		return nullptr;
		//*pkt = packets.front();
	}
	NetworkPacket *pkt = packets.front();
	packets.pop_front();
	doUnLock();
	return pkt;
}

void QueuedPacketsHelper::QueuePacket(NetworkPacket pkt) {
	doLock();
	packets.push_back(&pkt);
	doUnLock();
}

/* PacketStreamThread */
void PacketStreamThread::queue(NetworkPacket pkt) {
	mutex.lock();
	//verbosestream << FUNCTION_NAME << ": Queued packet <size=" << pkt.getSize() << ">, total: " << stream.size()+1 << std::endl;
	stream.push_back(pkt);
	mutex.unlock();
}

void PacketStreamThread::doSpecialPacket(std::unordered_map<u16, NetworkPacket> o_packet, u16 last_pkt_o_packet) {
	verbosestream << FUNCTION_NAME << ": !" << std::endl;
	if ((last_pkt_o_packet != 0) && (o_packet.find(last_pkt_o_packet) != o_packet.end())) {
		//verbosestream << FUNCTION_NAME << ": Pre" << std::endl;
		//for (u16 i = 1; i < last_pkt_o_packet+1; i++) {
		//	if (o_packet.find(i) == o_packet.end()) {
		////		verbosestream << FUNCTION_NAME << ": Transmission failed, no enough packets <F=" << i << ">" << std::endl;
		//		return; //Transmission not finished yet
		//	}
		//}
		//We can start building this packet, but on another thread while this keep running
		//Gotta make lambda
		verbosestream << FUNCTION_NAME << std::endl;
		auto lmb = [o_packet, last_pkt_o_packet] () {
			//This is being run in a lambda on a different thread
			NetworkPacket pkt;
			std::vector<u8> raw_data;
			raw_data.resize(2);
			uint32_t starting_point = 2;
			writeU16(&raw_data[0], 0x9a); //CMD
			
			for (u16 i = 1; i < last_pkt_o_packet+2; i++) {
				if (o_packet.find(i) != o_packet.end()) {
					NetworkPacket _pkt = o_packet.at(i);
					u8 *uint8_pointer = _pkt.getU8Ptr(0); //Assigned. [1] = Mode, [3] = Total Packets Verify, [5] = Order Count, [+5] Raw data
					raw_data.resize(raw_data.size()+_pkt.getSize());
					memcpy(&raw_data[(u32)starting_point], &uint8_pointer[5], 5000); //2
					starting_point += _pkt.getSize()-5;
					verbosestream << FUNCTION_NAME << ": Packet " << i << ", built! <size=" << starting_point << ", pr_size=" << _pkt.getSize() << ">" << std::endl;
				} else {
					errorstream << FUNCTION_NAME << ": Packets came on a corrupted form, throwing all packets" << std::endl;
				}
			}
			pkt.putRawPacket((u8*)raw_data.data(), raw_data.size(), (session_t)1);
			return pkt;
		};
		//QuickThreading<NetworkPacket, NetworkPacket> *_fish = nullptr;
		auto cb = [this] (QuickThreading<NetworkPacket, NetworkPacket> *QT, NetworkPacket pkt) {
			queue(pkt);
		};
		//Use QuickThreading
		QuickThreading<NetworkPacket, NetworkPacket> *func = new QuickThreading<NetworkPacket, NetworkPacket>(lmb, cb);
		func->start(); //This will destroy himself when run completely
		objects.push_back(func);
	}
}

//This has an interval of 20ms for every update sent by slave
void *PacketStreamThread::run() {
	verbosestream << FUNCTION_NAME << std::endl;
	std::deque<NetworkPacket> data;
	std::unordered_map<u16, NetworkPacket> _packets;
	u16 last_pkt_o_packet = 0;
	while (!stopRequested()) {
		//verbosestream << "f_loop" << std::endl;
		if (!objects.empty()) {
			if (objects.front()->ready()) {
				objects.front()->stop();
				delete objects.front();
				objects.pop_front();
			}
		}
		//May we copy the queue data?
		data.clear();
		mutex.lock();
		data = std::move(stream);
		stream.clear();
		mutex.unlock();
		if (data.size() > 0) {
			while (!data.empty()) {
				//verbosestream << "f_loop2" << std::endl;
				NetworkPacket central_pkt = data.front();
				data.pop_front();
				//We must remember that the packet may contain compressed packets to send here
				
				//u16 first_it = 2;
				u32 read_size = 3;
				u16 packet_count = 0;
				u8 mode = 1;
				central_pkt >> mode;
				if (mode > 0) { //Means the packet is coming divided
					u16 order;
					u16 is_last_packet;
					central_pkt >> is_last_packet >> order;
					verbosestream << FUNCTION_NAME << ": STREAM: Queued packet: " << order << ", lastpkt: " << is_last_packet << std::endl;
					_packets[order] = central_pkt; //Store with order variable
					last_pkt_o_packet = is_last_packet;
					bool failure = false;
					for (u16 i = 1; i < last_pkt_o_packet+1; i++) {
						if (_packets.find(i) == _packets.end()) {
							failure = true;
							//verbosestream << FUNCTION_NAME << ": STREAM: Packet count not finished: " << i << std::endl;
							break;
						}
					}
					if (!failure) {
						//verbosestream << FUNCTION_NAME << ": STREAM: Got!" << std::endl;
						doSpecialPacket(std::move(_packets), last_pkt_o_packet);
						last_pkt_o_packet = 0;
						_packets.clear();
					}
					continue;
				}
				central_pkt >> packet_count;
				
				verbosestream << "Packets to proccess: " << packet_count << ", CompressedPacketSize: " << central_pkt.getSize() << std::endl;
				if (packet_count > 500)
					warningstream << "Packets count in 1 packet overflow: " << packet_count << std::endl;
				
				
				for (u16 i = 0; i < packet_count; ++i) {
					//verbosestream << "f_loop_pkt_loop: " << i << std::endl;
					try {
						//Make a packet with controlled distance FIXME: deletes 2 bytes on stream
						//std::vector<u8> raw_data;
						u32 datasize = 0;
						central_pkt >> datasize;
						//verbosestream << "f_loop_pkt_loop[Packet]: " <<i<<": "<<datasize<<std::endl;
						const u8 *raw_data2 = central_pkt.getU8Ptr(read_size+4); //+4 = datasize(u32)
						//set central_pkt read offset
						central_pkt.increaseReadOffset(datasize); //PacketError FIX
						NetworkPacket pkt;
						//raw_data.resize(datasize);
						///memcpy(raw_data.data(), &raw_data2[0], datasize);
						//u8 *p = (u8*)&raw_data[0];
						pkt.putRawPacket(raw_data2, datasize, (session_t)0); //-4=Size (u32)
						//Lets see if packet size are the correct
						
						if (pkt.getSize() != datasize-2) {
							warningstream << "Packet size not correct: " << pkt.getSize() << " != " << datasize-2 << std::endl;
							read_size += datasize+2;
							continue;
						}
						verbosestream << FUNCTION_NAME << ": Packet: CMD=0x" << std::hex << pkt.getCommand() << std::dec << " with SIZE=" << pkt.getSize() << "/RAW=" << datasize << std::endl;//", Contents: \"" << std::string((char*)pkt.getString(0)) << "\"" << std::endl;
						SNE->DoApplyEnvironmentToPlayer(&pkt);
						//verbosestream << "f_loop_pkt_loop_try_got" << std::endl;
						read_size += datasize+4;
					} catch (PacketError &e) {
						warningstream << "Possible corrupted packet found: " << e.what() << std::endl;
						break;
					}
					//first_it = 0;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	actionstream << FUNCTION_NAME << ": Closing" << std::endl;
	return nullptr;
}

/* ServerNetworkEngine_NetworkThread */

//THREADING
ServerNetworkEngine_NetworkThread::~ServerNetworkEngine_NetworkThread() {
	
}

const uint32_t MAX_PACKET_SIZE = 64*1024*1024;
#include <cerrno>

void *ServerNetworkEngine_NetworkThread::run() {
	actionstream << "Occupied thread for networking, for slave ID: " << ServerID << std::endl;

	if (ServerData->SocketConn) { //active=Server is running
		while (!ServerData->active.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); //Wait for socket
			warningstream << "Waiting for subserver to be on" << std::endl;
		}
		//Connect to server [We act like a client]
		int sock_to_serv = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sock_to_serv == -1) {
			errorstream<<("Unable to create socket to connect slave server socket")<<std::endl;
			return nullptr;
		}
		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, ServerData->SocketDir.c_str(), sizeof(addr.sun_path) - 1);
		addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
		while (true) {
			if (connect(sock_to_serv, (sockaddr*)&addr, sizeof(addr)) == -1) {
				warningstream<<("Unable to connect to server socket, trying again in 500ms...{")<<ServerData->SocketDir<<"}{"<<addr.sun_path<<"}; Err: "<<strerror(errno)<<std::endl;
				//close(sock_to_serv);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			} else {
				break;
			}
		}

		ServerData->SocketReady = true;
		ServerData->socket_id = sock_to_serv;

		//Receive everytinh
		//All might be a packet simultaneously
		NetworkPacket pkt;
		uint32_t packet_count = 0;
		while (!stopRequested()) {
			pkt.clear();
			//Try getting a single packet
			std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
			ssize_t bytes_received = recv(sock_to_serv, &buffer[0], MAX_PACKET_SIZE, 0);
			if (bytes_received > 0) {
				//Direct compile
				actionstream << FUNCTION_NAME << ": Got bytes " <<bytes_received << ", from child" << std::endl;
				pkt.putRawPacket(buffer.data(), bytes_received, 0);
				jobDispatcher->SAVEFILE("/data_"+bytes_received, std::string(buffer.begin(), buffer.end()));
				jobDispatcher->DoApplyEnvironmentToPlayer(&pkt);
			}
		}
		return nullptr;
	}

	if (IndependientIPaddress) {
		actionstream << "Going to listen in a independient address" << std::endl;
		while (!stopRequested()) {
			NetworkPacket pkt;
			bool first = true;
			for (;;) {
				pkt.clear();
				try {
					if (first) {
						ServerData->BINDsrv_connection->Receive(&pkt);
						first = false;
					} else {
						if (!ServerData->BINDsrv_connection->TryReceive(&pkt))
							break;
					}
					//
					if (pkt.getCommand() == 0x9a)
						pst_thr->queue(pkt); //Might use multithreading for this
					else
						jobDispatcher->DoApplyEnvironmentToPlayer(&pkt);
				} catch (const con::InvalidIncomingDataException &e) {
					infostream << "ServerNetworkEngine_NetworkThread::run(): InvalidIncomingDataException: what()=" << e.what() << std::endl;
				} catch (const con::PeerNotFoundException &e) {
					// Do nothing
				} catch (const con::NoIncomingDataException &e) {
					break;
				}
			}
			//std::this_thread::sleep_for(std::chrono::milliseconds(8));
		}
	} else {
		actionstream << "Going to listen in the same address as the server" << std::endl;
		//Process queued items
		while (!stopRequested()) {
			NetworkPacket *pkt = jobDispatcher->PacketsQueue[ServerID]->getPacket();
			if (pkt != nullptr) {
				jobDispatcher->DoApplyEnvironmentToPlayer(pkt);
				delete pkt;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
}

/* ActiveSSThread */ //FIXME: Change ActiveSSThread to be AOMThread
void *ActiveSSThread::run() {
	verbosestream << FUNCTION_NAME << std::endl;
	s16 time;
	while (!stopRequested()) {
		//ActiveObjectMessages
		jb->mutex.lock();
		while (!jb->aomMsgs.empty()) {
			NetworkPacket pkt = jb->aomMsgs.front();
			jb->aomMsgs.pop_front();

			
			//SendRemoveAdd should be handled to register in it the ID that will be used here
			
			u16 msg_count = 0;
			u32 str_off;
			pkt >> msg_count;

			u16 obj_id, cmd = 0;
			u8 reliable = 0;
			u32 a, b = 0;

			for (u16 i = 0; i < msg_count; i++) {
				//pkt >> obj_id >> reliable >> b >> a >> cmd;
				pkt >> obj_id >> reliable >> cmd;
				//extract the message
				//std::string old_(pkt.getString(2+1+4+4+2+str_off), b);
				//std::string new_(pkt.getString(2+1+4+4+2+str_off+b), a);
				std::string old_ = pkt.readLongString(); //Use LongString method
				std::string new_ = pkt.readLongString();
				//str_off += a+b;
				std::string msg;
				//change values depending on the player
				for (auto it = jb->players.begin(); it != jb->players.end(); it++) {
					msg.clear();
					char idbuf[2];
					//Check message
					//Get the player ID for the SAO id
					u16 player_id = it->first;
					u16 id_for_player = SSO->GetObjectByPlayer(player_id, obj_id); //ID that the player has registered in his client
					bool proto_for_player = jb->players_protonet[player_id]; //Determines if player are old or new, new=true, old=false
					//Run checks
					if ((id_for_player == UNKNOWN_OBJECT_ID) || (id_for_player == NOT_KNOWN_PLAYER_ID)) {
						//The object are not found in the player, so we will queue it to see it later
						preObjQueue.push_back(_aom_double(player_id, obj_id, old_, new_, cmd, reliable));
						warningstream << "Object " << obj_id << " not found in player " << player_id << ", queuing" << std::endl;
						break; //Unknown object
					} else if (id_for_player == PLAYER_OBJECT_ID) {
						//id
						break;
					}
					//if (exists_player_sao && (cmd == AO_CMD_UPDATE_POSITION)) //Other than AO_CMD_UPDATE_POSITION are supported here
						//break; //Handled by multiserver_env
					//Edit updates so we can fit Sao's ID here
					if (cmd == AO_CMD_ATTACH_TO) {
						std::string temp_str;
						std::string c_temp_str;
						
						//ID
						writeU16((u8*) idbuf, id_for_player);
						temp_str.append(idbuf, sizeof(idbuf));
						c_temp_str.append(idbuf, sizeof(idbuf));
						bool failure = false;
						
						for (std::string str : {new_, old_}) {
							if (str.empty())
								continue;
							std::istringstream IS(str, std::ios::binary);
							//Adjust parent id
							u16 parentID = readS16(IS);
							std::string bone_str = deSerializeString16(IS);
							
							//u16 prt = SSO->GetObjectByPlayer(player_id, parentID);
							//if (prt == NOT_KNOWN_PLAYER_ID || prt == UNKNOWN_OBJECT_ID || prt == PLAYER_OBJECT_ID)
							//	continue;
							u16 prt = SSO->GetObjectByPlayer(player_id, parentID);
							if (prt == NOT_KNOWN_PLAYER_ID || prt == UNKNOWN_OBJECT_ID || prt == PLAYER_OBJECT_ID) {
								preObjQueue.push_back(_aom_double(player_id, obj_id, old_, new_, cmd, reliable));
								failure = true;
								break;
							}
							
							//Max than 37
							std::ostringstream os(std::ios::binary);
							writeU8(os, AO_CMD_ATTACH_TO);
							writeS16(os, prt);
							os << serializeString16(bone_str);
							writeV3F32(os, readV3F32(IS));
							writeV3F(os, readV3F32(IS), (proto_for_player ? 38 : 30));
							writeU8(os, readU8(IS));
							temp_str = os.str();
							
							//Not more than 37
							std::ostringstream os2(std::ios::binary);
							writeU8(os2, AO_CMD_ATTACH_TO);
							writeS16(os2, prt);
							os2 << serializeString16(bone_str);
							writeV3F1000(os2, readV3F1000(IS));
							writeV3F1000(os2, readV3F1000(IS));
							writeU8(os2, readU8(IS));
							c_temp_str = os2.str();
						}
						if (!failure) {
							//Prepare to send
							//Message
							msg = (proto_for_player ? temp_str : c_temp_str);
							NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES, msg.size(), it->second);
							pkt.putRawString(msg.c_str(), msg.size());
							jb->SNE->Server___->QueueSendTo(it->second, pkt, true);
						}
					} else {
						//Normal sending
						
						writeU16((u8*) idbuf, id_for_player);
						msg.append(idbuf, sizeof(idbuf));
						msg.append(proto_for_player ? new_ : old_, (proto_for_player ? new_ : old_).size());
						
						NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES, msg.size(), it->second);
						pkt.putRawString(msg.c_str(), msg.size());
						jb->SNE->Server___->QueueSendTo(it->second, pkt, true);
					}
				}
			}
		}
		jb->mutex.unlock();
		time+=10;
		if (time>1000) {
			time=0;
			//Send packets to players with his objects
			std::deque<_aom_double> overwrite;
			std::deque<_aom_double>::iterator it = preObjQueue.begin();
			while (it != preObjQueue.end()) {
				_aom_double data = *it;
				char idbuf[2];
				if ((SSO->GetObjectByPlayer(data.player, data.obj) != UNKNOWN_OBJECT_ID) && (SSO->GetObjectByPlayer(data.player, data.obj) != NOT_KNOWN_PLAYER_ID)) {
					//Object already identified, send modifications
					u16 id_for_player = SSO->GetObjectByPlayer(data.player, data.obj); //ID for object in player
					bool proto_for_player = jb->players_protonet[data.player]; //Determine if the client are old or new
					u16 cmd = data.cmd;
					std::string msg;
					//First check
					if (cmd == AO_CMD_ATTACH_TO) {
						std::string temp_str;
						std::string c_temp_str;
						//ID
						writeU16((u8*) idbuf, id_for_player);
						temp_str.append(idbuf, sizeof(idbuf));
						c_temp_str.append(idbuf, sizeof(idbuf));
						bool failure = false;
						
						for (std::string str : {data._new, data._old}) {
							if (str.empty())
								continue;
							std::istringstream IS(str, std::ios::binary);
							//Adjust parent id
							u16 parentID = readS16(IS);
							std::string bone_str = deSerializeString16(IS);
							
							u16 prt = SSO->GetObjectByPlayer(data.player, parentID);
							if (prt == NOT_KNOWN_PLAYER_ID || prt == UNKNOWN_OBJECT_ID || prt == PLAYER_OBJECT_ID) {
								overwrite.push_back(data);
								failure = true;
								break;
							}
							
							//Max than 37
							std::ostringstream os(std::ios::binary);
							writeU8(os, AO_CMD_ATTACH_TO);
							writeS16(os, prt);
							os << serializeString16(bone_str);
							writeV3F32(os, readV3F32(IS));
							writeV3F(os, readV3F32(IS), (proto_for_player ? 38 : 30));
							writeU8(os, readU8(IS));
							temp_str = os.str();
							
							//Not more than 37
							std::ostringstream os2(std::ios::binary);
							writeU8(os2, AO_CMD_ATTACH_TO);
							writeS16(os2, prt);
							os2 << serializeString16(bone_str);
							writeV3F1000(os2, readV3F1000(IS));
							writeV3F1000(os2, readV3F1000(IS));
							writeU8(os2, readU8(IS));
							c_temp_str = os2.str();
						}
						if (failure) {
							it++;
							continue;
						}
						//Prepare to send
						//Message
						msg = (proto_for_player ? temp_str : c_temp_str);
						NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES, msg.size(), jb->players.at(data.player));
						pkt.putRawString(msg.c_str(), msg.size());
						jb->SNE->Server___->QueueSendTo(jb->players.at(data.player), pkt, true);
					} else {
						writeU16((u8*) idbuf, id_for_player);
						msg.append(idbuf, sizeof(idbuf));
						msg.append(proto_for_player ? data._new : data._old, (proto_for_player ? data._new : data._old).size());
						
						NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES, msg.size(), jb->players.at(data.player));
						pkt.putRawString(msg.c_str(), msg.size());
						jb->SNE->Server___->QueueSendTo(jb->players.at(data.player), pkt, true);
					}
				} else if (SSO->GetObjectByPlayer(data.player, data.obj) == NOT_KNOWN_PLAYER_ID) { //This have been deleted
					//Do nothing
				} else {
					overwrite.push_back(data); //Its unknown to player, and the player still playing, continue this for later
				}
				delete[] idbuf;
				it++;
			}
			//Any problems will be queued again to fix
			preObjQueue = overwrite;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}



//SC_H
SC_H::SC_H(u16 player_id, u16 srv): player(player_id), server(srv), time((s16)0) {
	verbosestream << FUNCTION_NAME << ": Player: " << player_id << ", Server:" << srv << ", time: 0" << std::endl;
}

//SendConnect thread + xtra functions
_SNE_SC_THR_CLASS::_SNE_SC_THR_CLASS(ServerNetworkEngine *ptr): SNE(ptr), Thread("_SNE_SC_THR_CLASS") {
}

void _SNE_SC_THR_CLASS::lock() {
	mutex.lock();
}

void _SNE_SC_THR_CLASS::unlock() {
	mutex.unlock();
}

void _SNE_SC_THR_CLASS::queue_sc(u16 player_id, u16 srv) {
	verbosestream << FUNCTION_NAME << std::endl;
	//Get the id for the sendconnect
	register u16 to_use = 0;
	
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<u16> dist(0, UINT16_MAX-1);
	bool got = false;
	while (!got) {
		register u16 id = dist(gen);
		if (sc_s.find(id) == sc_s.end()) {
			to_use = id;
			break;
		}
	}
	SC_H val = SC_H(player_id, srv); //already assigned an s16 value
	val.time = 3000;
	sc_s[to_use] = val;
}

void _SNE_SC_THR_CLASS::send(const SC_H data) {
	verbosestream << FUNCTION_NAME << std::endl;
	u16 player = data.player;
	u16 s_id = data.server;
	//Send
	SNE->_HELPER_SendConnect(player, s_id);
}

void *_SNE_SC_THR_CLASS::run() {
	verbosestream << FUNCTION_NAME << std::endl;
	while (!stopRequested()) {
		lock();
		_BEING_USED = true;
		std::vector<u16> queue;
		//3000ms = 3s
		for (auto it = sc_s.begin(); it != sc_s.end(); it++) { //identifier, values
			it->second.time -= 20; //s16
			if ((it->second.time <= 2300) && (it->second.time >= 2100) && (it->second.phases[0] != true)) {
				SNE->SendActivateBackground(it->second.player);
				it->second.phases[0] = true;
				//send(it->second);
			} else if ((it->second.time <= 400) && (it->second.time >= 100) && (it->second.phases[1] != true)) {
				SNE->SendDeactivateBackground(it->second.player);
				it->second.phases[1] = true;
			} else if ((it->second.time <= 99) && (it->second.time >= 0) && (it->second.phases[2] != true)) {
				it->second.phases[2] = true;
				send(it->second);
				queue.push_back(it->first);
			}
		}
		for (const u16 val : queue) {
			sc_s.erase(val);
		}
		_BEING_USED = false;
		unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

/* Media Management */
#include <filesys.h>
#include <fstream>

/* _SNE_FILESYSTEM <ServersNetworkEngine> */
void _SNE_FILESYSTEM::LOAD_FILESYSTEM(const std::string &path) {
	minestars_folder_path = path + "/MineStars";
	bool _state_dir = fs::CreateAllDirs(path);
	FATAL_ERROR_IF(!_state_dir, "Could not create directory for MineStars Server Network Engine, be sure if theres enough space in device & check if correct permissions");
	//TODO: May make plugins support?
	std::string loging = "\n# MINESTARS #\n";
	SAVEFILE("/servernet.log", loging);
	verbosestream << FUNCTION_NAME << std::endl;
}
void _SNE_FILESYSTEM::SAVEFILE(const std::string &path, const std::string &raw) {
	verbosestream << FUNCTION_NAME << ": " << minestars_folder_path << "" << path << std::endl;
	std::string huh;
	huh.append(minestars_folder_path);
	huh.append(path);
	//{
	//	std::string d_huh = path;
	//	d_huh.append("/..");
	//	CHECKFOLDER(d_huh.c_str());
	//}
	std::ofstream file(huh.c_str(), std::ios_base::binary | std::ios_base::trunc);
	file.write(raw.c_str(), raw.length());
	file.close();
	//FATAL_ERROR_IF(file.fail(), "Could not write file: " + path.c_str() + ", check permissions & storage");
}
void _SNE_FILESYSTEM::LOADFILE(const std::string &path, std::ostringstream &data, bool &status) {
	std::string huh = minestars_folder_path + path;
	//huh.append(minestars_folder_path);
	//huh.append(path);
	std::ifstream fis(huh.c_str(), std::ios_base::binary);
	if (!fis.good()) {
		warningstream << FUNCTION_NAME << ": Going to access on a unknown file: " << huh << std::endl;
		status = false;
		return;
	}
	bool bad = false;
	char buf[4096];
	for(;;){
		fis.read(buf, sizeof(buf));
		std::streamsize len = fis.gcount();
		data.write(buf, len);
		if(fis.eof())
			break;
		if(!fis.good()){
			bad = true;
			break;
		}
	}
	if (bad)
		warningstream << FUNCTION_NAME << ": Failed to read file: " << path << std::endl;
	status = true;
}
void _SNE_FILESYSTEM::CHECKFOLDER(const std::string &path) {
	verbosestream << FUNCTION_NAME << ": bool:" << ((bool)fs::CreateAllDirs(minestars_folder_path + path)) << std::endl;
}
bool _SNE_FILESYSTEM::EXISTSFILE(const std::string &path) {
	std::ifstream fis(((std::string)(minestars_folder_path + path)).c_str(), std::ios_base::binary);
	return fis.good();
}
void _SNE_FILESYSTEM::REMOVEFILE(const std::string &path) {
	verbosestream << FUNCTION_NAME << ": " << (minestars_folder_path + path) << ", " << ((bool)fs::RecursiveDelete(minestars_folder_path + path)) << std::endl;
}

// We gotta save 'sha1hex = name' (In binary mode)
// A folder will be created in the world's folder, for storing media
//Structure:
/*
WorldFolder:
    |
    #-->MineStars (Config place)
            |
            #-->SlaveServersMedia
            |         |
            |         #-->Slave_1
            |                |
            |                #-->'Stored media of slaves'
            #-->SlaveServersMediaMeta
                        |
                        #-->Slave_1.msd (MineStars SHA1 Data)
*/

/* _SNE_MEDIA */
void _SNE_MEDIA::runMMS() {
	state_verif = new BoolField<u16>;
}

//Need use of Server object for checks
u32 _SNE_MEDIA::getTotalMediaCount() {
	//if (!_CFG_SRVSTATE)
	//	warningstream << FUNCTION_NAME << " called on slave server";
	return media.size();
}
bool _SNE_MEDIA::loadMediaNcheckSums(u16 ID, NetworkPacket *pkt) {
	verbosestream << FUNCTION_NAME << std::endl;
	if (state_verif->at(ID))
		return false;
	state_verif->switch_values(ID);
	//Going to get all items of the packet
	std::unordered_map<std::string, std::string> checks;
	u16 size_ = 0;
	*pkt >> size_;
	for (short unsigned int i = 0; i < size_; i++) {
		std::string name, _sha1;
		*pkt >> name >> _sha1;
		//verbosestream << FUNCTION_NAME << ": " << name << " <> " << _sha1 << " <> " << i << ":" << size_ << std::endl;
		checks[name] = _sha1;
	}
	
	std::stringstream ss;
	ss << ID;
	
	//Engine to check media, compare the old files with the new ones
	uint32_t changed_files = 0;
	uint32_t modified_files = 0;
	for (const auto &i: checks) {
		if (!SNE->ServerHasMedia(i.first)) { //Server media should be high priorized, otherwise the slaves will had full control of modifications
			if (sha1_storage[ID].find(i.first) == sha1_storage[ID].end()) {
				//Media is new, add to change list to save
				to_getmedia[ID].push_back(i.first);
				changed_files++;
			} else if (sha1_storage[ID].at(i.first) != i.second) { //Check if the media contents changed with sha1 verification (i.second = sha1, i.first = name)
				modified_files++;
				to_getmedia[ID].push_back(i.first);
			} else {
				//Pass, add into server media
				std::string _path = SNEF->minestars_folder_path + "/SlaveServersMedia/Slave_" + ss.str() + "/" + i.first;
				media[i.first] = MediaInfo(_path, i.second);
			}
		}
	}

	verbosestream << "New files: " << changed_files << ", modified files: " << modified_files << std::endl;
	
	//Check for removed media
	std::vector<std::string> todel;
	for (const auto &i: sha1_storage[ID]) {
		//This will fix memory overflow due to media that has been deleted in the slave side
		if (checks.find(i.first) == checks.end()) { //Means the slave has deleted some media
			//Remove from all sources
			media.erase(i.first);
			todel.push_back(i.first); //sha1_storage
			//to_getmedia[ID][]; //unsupported
			to_delete[ID][i.first] = true;
			//Delete file
			SNEF->REMOVEFILE("/SlaveServersMedia/Slave_" + ss.str() + "/" + i.first);
		}
	}
	
	return !to_getmedia[ID].empty();
}
NetworkPacket _SNE_MEDIA::sendGetMedia(u16 ID) {
	if (to_getmedia[ID].empty())
		return NetworkPacket();
	NetworkPacket pkt(0x110, 0);
	pkt << (u16) to_getmedia[ID].size();
	for (const std::string &name: to_getmedia[ID]) {
		pkt << name;
	}
	return pkt;
}

void _SNE_MEDIA::setSNEF(_SNE_FILESYSTEM *obj) { SNEF = obj; }

static std::string getsha1(const char *rawdata, u16 lengh) {
	SHA1 sha1;
	sha1.addBytes(rawdata, lengh);
	unsigned char *digest = sha1.getDigest();
	std::string sha1_base64 = base64_encode(digest, 20);
	free(digest);
	return sha1_base64;
}

///struct __two {
//	std::string _1;
//	std::string _2;
//}

bool _SNE_MEDIA::HasMedia(const std::string &name) {
	return media.find(name) != media.end();
}

void _SNE_MEDIA::startLoadSlaveMedia(u16 ID, NetworkPacket *pkt) {
	//pkt can be nullptr if theres no media to download
	//This will get all media sent by slave to save here
	//Also this will save the info of SHA1s
	std::unordered_map<std::string, std::string> to_fetch;
	if (!to_getmedia[ID].empty()) {
		//Format:
		//[0-2] Count
		//[2-6] Lengh of file
		//[inf] file
		std::stringstream ss;
		ss << ID;
		//SNEF->CHECKFOLDER(ss.str());
		u16 count = 0;
		*pkt >> count;
		std::string str(pkt->getString(7), pkt->getSize()-7);
		std::istringstream is(str, std::ios_base::binary);
		SNEF->CHECKFOLDER("/SlaveServersMedia/Slave_" + ss.str());
		for (u16 i = 0; i < count; i++) {
			std::string name = deSerializeString16(is);
			verbosestream << FUNCTION_NAME << ": Getting " << name << std::endl;
			std::string data = deSerializeString64(is);
			std::string path = "/SlaveServersMedia/Slave_" + ss.str() + "/" + name;
			SNEF->SAVEFILE(path, data);
			std::string rpath = SNEF->minestars_folder_path + path;
			std::string sha1_ = getsha1(data.c_str(), data.size());
			media[name] = MediaInfo(rpath, sha1_);
			//return;
		}
		//Save the media info
		//Must get all info and then write
		SNEF->CHECKFOLDER("/SlaveServersMediaMeta");
		//if (!EXISTSFILE(ss.str())) {
			//Lets make a file from zero!
			//Lets use to_getmedia for reference
			std::unordered_map<std::string, std::string> cache;
			for (const std::string &str: to_getmedia[ID]) {
				//Get sha1
				bool status = false;
				std::ostringstream os(std::ios_base::binary);
				std::string path = "/SlaveServersMedia/Slave_" + ss.str() + "/" + str;
				SNEF->LOADFILE(path, os, status);
				if (!status) {
					warningstream << FUNCTION_NAME << ": File " << str << " not found while saving media" << std::endl;
					continue;
				}
				std::string s_ = os.str();
				std::string sha = getsha1(s_.c_str(), s_.size());
				cache[str] = sha;
				//media_verif[ID].define(str, true);
			}
			
			//Include old media, as we go, because we required the new media, not all media
			for (const auto &i: sha1_storage[ID]) {
				if (cache.find(i.first) == cache.end()) { //Not known data found
					verbosestream << FUNCTION_NAME << ": [FromFile] Including " << i.first << std::endl;
					cache[i.first] = i.second;
				}
			}
			
			//Due from ostringstream we can not push raw uint16_t because it will write in the wrong way, as we need a perfect raw byte order
			//So we will use a string+buffer helper
			char _buf[2];
			std::string _bufh;
			_bufh.reserve(2);
			writeU16((u8*)&_buf[0], (u16)cache.size());
			_bufh.append(_buf, 2);
			
			u16 from_deletion = 0;
			
			std::ostringstream os(std::ios_base::binary);
			os << serializeString16(std::string("MineStars"));
			os << _bufh;
			for (const auto &i: cache) {
				//verbosestream << FUNCTION_NAME << ": Saving: " << i.first << ", sha1=" << i.second << std::endl;
				if (to_delete[ID].find(i.first) != to_delete[ID].end()) {
					from_deletion++;
					verbosestream << FUNCTION_NAME << ": To delete: " << i.first << std::endl;
					continue; //Don't include deleted media
				}
				os << serializeString16(i.second);
				os << serializeString16(i.first);
			}
			//Save the checksums file
			std::string _path = "/SlaveServersMediaMeta/Slave_" + ss.str() + ".msd";
			SNEF->SAVEFILE(_path, os.str());
			verbosestream << FUNCTION_NAME << ": Entries: " << (cache.size()-from_deletion) << std::endl;
		//} else {
		//	
		//}
		//Wipe vector on to_getmedia
		to_getmedia[ID].clear();
		cache.clear();
		
	} else {
		//Nothing to modify, just upload media to media[]
		std::stringstream ss;
		ss << ID;
		for (const auto &i: sha1_storage[ID]) {
			std::string path = SNEF->minestars_folder_path + "/SlaveServersMedia/Slave_" + ss.str() + "/" + i.first;
			media[i.first] = MediaInfo(path, i.second);
		}
		//Wipe vector on to_getmedia
		to_getmedia[ID].clear();
	}
	
	
}
//u16 servers[] = {1,2,3,4}, free()
void _SNE_MEDIA::loadMeta(u16 max_count) {
	SNEF->CHECKFOLDER("/SlaveServersMediaMeta");
	for (u16 i = 1; i < max_count+1; i++) {
		bool exists = false;
		std::ostringstream os(std::ios_base::binary);
		std::stringstream ss;
		ss << "/SlaveServersMediaMeta/Slave_";
		ss << i;
		ss << ".msd";
		SNEF->LOADFILE(ss.str(), os, exists);
		to_getmedia[i] = std::vector<std::string>();
		sha1_storage[i] = std::unordered_map<std::string, std::string>();
		//media_verif[i] = BoolField<std::string>();
		
		if (!exists) {
			warningstream << "Path: " << ss.str() << " do not exist, skipping";
			continue;
		} else {
			verbosestream << FUNCTION_NAME << ": Loading config: " << i << std::endl; //In the most illegal form
			//The file is an MSD format (binary) so we will seek 2+ bytes
			//[0] to [11] = MineStars+2bytes lengh
			//[11] to [13] = Fields
			//[13] to [I] = Data
			//Structure:
			//[13] to [13+2] = Lengh of next SHA1 string
			//[13+2] to [13+X] = String SHA1
			//[13+2+X] to [13+2+X+2] = Lengh of the name for media
			//[13+4+X] to [13+4+X+Y] = String NAME
			//std::istringstream file(os.str(), std::ios_base::binary);
			
			u32 readBUF = 0;
			bool _1 = false;
			bool state = false;
			std::string sha, name;
			char buffer[80] = {0};
			std::string stringbuf = os.str();
			
			auto checkforerrors = [i] (bool _) {
				if (_) {
					errorstream << "_SNE_MEDIA::loadMeta(): Something failed while loading the config: Slave_" << i << ".msd, the file may is corrupted, aborting" << std::endl;
				}
			};
			
			//Will use other method of reading than deserializestring and readu16
			
			
			//size_t sizeBUF = stringbuf.size();
			//u32 buffer_reading_off = 0;
			
			auto readstring = [this, &readBUF, &stringbuf, &buffer, &state] (bool &failure) {
				//Clear buffer
				memset(buffer, 0, sizeof(buffer));
				//Read lenght
				//Will use generic byte-swapping for this time
				u16 _LEN = ((u16)stringbuf[readBUF] << 8) | ((u16)stringbuf[readBUF+1] << 0); //990
				verbosestream << FUNCTION_NAME << ": StringLenght=" << _LEN << std::endl;
				state = (_LEN >= 80);
				failure = (_LEN >= 80);
				if (state)
					return std::string("");
				memcpy(buffer, stringbuf.data() + (readBUF+2), _LEN);
				buffer[_LEN] = '\0'; //Endpoint
				readBUF += _LEN+2;
				return std::string(buffer, _LEN);
			};
			
			bool ___;
			verbosestream << FUNCTION_NAME << ": " << readstring(___) << std::endl;
			u16 fields = ((u8)stringbuf[readBUF] << 8) | ((u8)stringbuf[readBUF+1] << 0);
			readBUF += 2; //Seeking 2 bytes, for some reason the file data saver pushes 4 bytes by no reason, making sometimes crash, so i've put 4 bytes
			if (state) {
				checkforerrors(true);
				continue;
			}
			//Start
			checkforerrors(fields > (UINT16_MAX/2)+12000); //Sometimes the data might be corrupted

			uint32_t __count = 0;

			for (u16 i2 = 0; i2 < fields; i2++) {
				sha.clear();
				name.clear();
				bool failure = false;
				
				sha = readstring(failure);
				//verbosestream << sha << std::endl;
				if (failure) {
					_1 = true;
					break;
				}
				name = readstring(failure);
				//verbosestream << name << std::endl;
				if (failure) {
					_1 = true;
					break;
				}
				//Store
				sha1_storage[i][name] = sha;
				//verbosestream << FUNCTION_NAME << ": Loaded: " << name << std::endl;

				__count++;
				
				//State Verif
				//media_verif[i].define(name, true);
			}

			verbosestream << FUNCTION_NAME << ": Loaded " << __count << " files" << std::endl;

			if (_1) {
				//verbosestream << "f" << std::endl;
				checkforerrors(true);
				//verbosestream << "c" << std::endl;
				//continue;
			}
		}
		
	}
}


//TODO: May make a subsystem which grabs the Registered item to the main proxy server, for loading new nodes from different slave servers, allowing pure modifications and fun
//FIXME: Done





















