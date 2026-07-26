/*
Minetest
Copyright (C) 2015 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <unordered_map>
#include <sstream>
#include <iostream>

#include "chatmessage.h"
#include "serverenvironment.h"
#include "server.h"
#include "log.h"
#include "emerge.h"
#include "mapblock.h"
#include "nodedef.h"
#include "remoteplayer.h"
#include "rollback_interface.h"
#include "scripting_server.h"
#include "settings.h"
#include "tool.h"
#include "version.h"
#include "network/mt_connection.h"
#include "network/networkprotocol.h"
#include "network/serveropcodes.h"
#include "server/player_sao.h"
#include "server/serverinventorymgr.h"
#include "util/auth.h"
#include "util/base64.h"
#include "util/pointedthing.h"
#include "util/serialize.h"
#include "util/srp.h"
#include "ServerNetworkEngine.h"
#include "slave_helpers.h"
#include "slave_proxy_net/objects_id_logic.h"
#include "slave_proxy_net/definitions_jumper.h"
#include "core/NetworkEngine.h"
#include "../addons/cblks_def.hpp"
#include "../core/handles.h"

#include "itemdef.h"
#include "nodedef.h"
#include "craftdef.h"
#include "emerge.h"

void Server::handleCommand_Deprecated(NetworkPacket* pkt)
{
	infostream << "Server: " << toServerCommandTable[pkt->getCommand()].name
		<< " not supported anymore" << std::endl;
}

void Server::handleCommand_Init(NetworkPacket* pkt)
{

	if(pkt->getSize() < 1)
		return;

	session_t peer_id = pkt->getPeerId();

	Address addr;
	std::string addr_s;
	try {
		addr = m_con->GetPeerAddress(peer_id);
		addr_s = addr.serializeString();
	} catch (con::PeerNotFoundException &e) {
		/*
		 * no peer for this packet found
		 * most common reason is peer timeout, e.g. peer didn't
		 * respond for some time, your server was overloaded or
		 * things like that.
		 */
		infostream << "Server::ProcessData(): Canceling: peer " << peer_id <<
			" not found" << std::endl;
		return;
	}
	
	if (ServersNetworkObject->AreSlave) {
		errorstream << "Declined handleCommand_Init(PKT) sent from: " << addr_s << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_SLAVESERVER);
		return;
	}
	
	RemoteClient *client = getClient(peer_id, CS_Created);

	if (client->getState() > CS_Created) {
		verbosestream << "Server: Ignoring multiple TOSERVER_INITs from " <<
			addr_s << " (peer_id=" << peer_id << ")" << std::endl;
		return;
	}

	client->setCachedAddress(addr);

	verbosestream << "Server: Got TOSERVER_INIT from " << addr_s <<
		" (peer_id=" << peer_id << ")" << std::endl;

	// First byte after command is maximum supported
	// serialization version
	u8 client_max;
	u16 supp_compr_modes;
	u16 min_net_proto_version = 0;
	u16 max_net_proto_version;
	std::string playerName;

	*pkt >> client_max >> supp_compr_modes >> min_net_proto_version
			>> max_net_proto_version >> playerName;

	u8 our_max = SER_FMT_VER_HIGHEST_READ;
	// Use the highest version supported by both
	u8 depl_serial_v = std::min(client_max, our_max);
	// If it's lower than the lowest supported, give up.
	if (depl_serial_v < SER_FMT_VER_LOWEST_READ)
		depl_serial_v = SER_FMT_VER_INVALID;

	if (depl_serial_v == SER_FMT_VER_INVALID) {
		actionstream << "Server: A mismatched client tried to connect from " <<
			addr_s << " ser_fmt_max=" << (int)client_max << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_WRONG_VERSION);
		return;
	}

	client->setPendingSerializationVersion(depl_serial_v);

	/*
		Read and check network protocol version
	*/

	u16 net_proto_version = 0;

	// Figure out a working version if it is possible at all
	if (max_net_proto_version >= SERVER_PROTOCOL_VERSION_MIN ||
			min_net_proto_version <= SERVER_PROTOCOL_VERSION_MAX) {
		// If maximum is larger than our maximum, go with our maximum
		if (max_net_proto_version > SERVER_PROTOCOL_VERSION_MAX)
			net_proto_version = SERVER_PROTOCOL_VERSION_MAX;
		// Else go with client's maximum
		else
			net_proto_version = max_net_proto_version;
	}

	verbosestream << "Server: " << addr_s << ": Protocol version: min: "
			<< min_net_proto_version << ", max: " << max_net_proto_version
			<< ", chosen: " << net_proto_version << std::endl;

	client->net_proto_version = net_proto_version;

	const u16 server_proto_ver_min = g_settings->getBool("enable_protocol_compat") ? SERVER_PROTOCOL_VERSION_MIN : SERVER_PROTOCOL_VERSION_MIN_NOCOMPAT;
	if ((g_settings->getBool("strict_protocol_version_checking") &&
			net_proto_version != LATEST_PROTOCOL_VERSION) ||
			net_proto_version < server_proto_ver_min ||
			net_proto_version > SERVER_PROTOCOL_VERSION_MAX) {
		actionstream << "Server: A mismatched client tried to connect from " <<
			addr_s << " proto_max=" << (int)max_net_proto_version << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_WRONG_VERSION);
		return;
	}

	/*
		Validate player name
	*/
	const char* playername = playerName.c_str();

	size_t pns = playerName.size();
	if (pns == 0 || pns > PLAYERNAME_SIZE) {
		actionstream << "Server: Player with " <<
			((pns > PLAYERNAME_SIZE) ? "a too long" : "an empty") <<
			" name tried to connect from " << addr_s << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_WRONG_NAME);
		return;
	}

	if (!string_allowed(playerName, PLAYERNAME_ALLOWED_CHARS)) {
		actionstream << "Server: Player with an invalid name tried to connect "
			"from " << addr_s << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_WRONG_CHARS_IN_NAME);
		return;
	}

	m_clients.setPlayerName(peer_id, playername);
	//TODO (later) case insensitivity

	std::string legacyPlayerNameCasing = playerName;
	if (false){
		std::string reason;
		//m_script->on_prejoinplayer(playername, addr_s, &reason)
		if ((reinterpret_cast<bool(*)(const char*, std::string, std::string*)>(AddonsCallbacks[CALLBACK_ON_PREJOINPLAYER]))(playername, addr_s, &reason)) {
			actionstream << "Server: Player with the name \"" << playerName <<
				"\" tried to connect from " << addr_s <<
				" but it was disallowed for the following reason: " << reason <<
				std::endl;
			DenyAccess(peer_id, SERVER_ACCESSDENIED_CUSTOM_STRING, reason);
			return;
		}
	}
//abort();
	infostream << "Server: New connection: \"" << playerName << "\" from " <<
		addr_s << " (peer_id=" << peer_id << ")" << std::endl;

	// Enforce user limit.
	// Don't enforce for users that have some admin right or mod permits it.
	if (m_clients.isUserLimitReached() && playername != g_settings->get("name") && !(reinterpret_cast<bool(*)(const char*, std::string)>(AddonsCallbacks[CALLBACK_ON_PLAYEREVENT]))(playername, addr_s)) {
		actionstream << "Server: " << playername << " tried to join from " <<
			addr_s << ", but there are already max_users=" <<
			g_settings->getU16("max_users") << " players." << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_TOO_MANY_USERS);
		return;
	}

	/*
		Compose auth methods for answer
	*/
	std::string encpwd; // encrypted Password field for the user
	bool has_auth = getAuth(playername, &encpwd, nullptr, nullptr);
	u32 auth_mechs = 0;

	client->chosen_mech = AUTH_MECHANISM_NONE;

	if (has_auth) {
		std::vector<std::string> pwd_components = str_split(encpwd, '#');
		if (pwd_components.size() == 4) {
			if (pwd_components[1] == "1") { // 1 means srp
				auth_mechs |= AUTH_MECHANISM_SRP;
				client->enc_pwd = encpwd;
			} else {
				actionstream << "User " << playername << " tried to log in, "
					"but password field was invalid (unknown mechcode)." <<
					std::endl;
				DenyAccess(peer_id, SERVER_ACCESSDENIED_SERVER_FAIL);
				return;
			}
		} else if (base64_is_valid(encpwd)) {
			auth_mechs |= AUTH_MECHANISM_LEGACY_PASSWORD;
			client->enc_pwd = encpwd;
		} else {
			actionstream << "User " << playername << " tried to log in, but "
				"password field was invalid (invalid base64)." << std::endl;
			DenyAccess(peer_id, SERVER_ACCESSDENIED_SERVER_FAIL);
			return;
		}
	} else {
		std::string default_password = g_settings->get("default_password");
		if (default_password.length() == 0) {
			auth_mechs |= AUTH_MECHANISM_FIRST_SRP;
		} else {
			// Take care of default passwords.
			client->enc_pwd = get_encoded_srp_verifier(playerName, default_password);
			auth_mechs |= AUTH_MECHANISM_SRP;
			// Allocate player in db, but only on successful login.
			client->create_player_on_auth_success = true;
		}
	}

	/*
		Answer with a TOCLIENT_HELLO
	*/

	verbosestream << "Sending TOCLIENT_HELLO with auth method field: "
		<< auth_mechs << std::endl;

	NetworkPacket resp_pkt(TOCLIENT_HELLO,
		1 + 4 + legacyPlayerNameCasing.size(), peer_id);

	u16 depl_compress_mode = NETPROTO_COMPRESSION_NONE;
	resp_pkt << depl_serial_v << depl_compress_mode << net_proto_version
		<< auth_mechs << legacyPlayerNameCasing;

	Send(&resp_pkt);

	client->allowed_auth_mechs = auth_mechs;
	client->setDeployedCompressionMode(depl_compress_mode);

	m_clients.event(peer_id, CSE_Hello);
}

void Server::handleCommand_Init2(NetworkPacket* pkt)
{
	//NOTE: About this function, please don't use m_env->getPlayer(uint16_t); use raw methods as the client din't send yet a StageTwo request for completed initialization
	session_t peer_id = pkt->getPeerId();
	verbosestream << "Server: Got TOSERVER_INIT2 from " << peer_id << std::endl;

	if (ServersNetworkObject->AreSlave) {
		errorstream << "Declined handleCommand_Init2(PKT)" << std::endl;
		return;
	}

	m_clients.event(peer_id, CSE_GotInit2);
	u16 protocol_version = m_clients.getProtocolVersion(peer_id);

	std::string lang;
	if (pkt->getSize() > 0)
		*pkt >> lang;

	/*
		Send some initialization data
	*/

	infostream << "Server: Sending content to " << peer_id << std::endl;

	// Send item definitions
	SendItemDef(peer_id, m_itemdef, protocol_version);

	// Send node definitions
	SendNodeDef(peer_id, m_nodedef, protocol_version);

	m_clients.event(peer_id, CSE_SetDefinitionsSent);

	// Send media announcement
	sendMediaAnnouncement(peer_id, lang);

	RemoteClient *client = getClient(peer_id, CS_InitDone);
	
	client->setLangCode(lang);

	// Warnings about protocol version can be issued here
	if (client->net_proto_version < LATEST_PROTOCOL_VERSION) {
		SendChatMessage(peer_id, ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
			L"# Server: WARNING: YOUR CLIENT'S VERSION MAY NOT BE FULLY COMPATIBLE "
			L"WITH THIS SERVER!"));
	}
}

using namespace std;

//Connect player
void Server::handleCommand_GotConnect(NetworkPacket* pkt) {
	if (pkt->getSize() < 9) {
		warningstream << "Packet are not complete 'handleCommand_GotConnect'" << std::endl;
		return;
	}
	/*
		[0] COMMAND
		[1] u16 2 PREDEFINED PLAYER ID
		[2] std::string (unknown amount of bytes) PREDEFINED PLAYER NAME
		[3] u16 2 MINIMUM PROTOCOL
		[4] u16 2 MAXIMUM PROTOCOL
		[5] u16 2 PREDEFINED PROTOCOL
		[? 6] std::string (unknown amount of bytes) CLIENT VERSION
		[? 7] std::string (unknown amount of bytes) PLATFORM VERSION (Freeminer, Minetest, Luanti, Multicraft, Minetest-c55, Rollertest, Kawaii Minecraft, MineStars (this))
	*/
	u8 SRZ_VER;//ClientDataHelper
	u16 ID; //ClientDataHelper&RemotePlayer
	u16 M_PROTO; //RemotePlayer
	u16 MM_PROTO; //RemotePlayer
	u16 P_PROTO; //ClientDataHelper
	u16 SAO_ID; //ClientDataHelper & RemotePlayer
	std::string name;
	std::string ver;
	std::string platform;
	
	
	*pkt >> ID;
	*pkt >> M_PROTO;
	*pkt >> MM_PROTO;
	*pkt >> P_PROTO;
	*pkt >> SRZ_VER;
	*pkt >> SAO_ID;
	*pkt >> name;
	*pkt >> platform;
	*pkt >> ver;

	//const char *CL_VER = ver.c_str(); //ClientDataHelper
	//const char *PLT_VER = platform.c_str(); //ClientDataHelper
	const char *PNAME = name.c_str(); //ClientDataHelper
	
	//sys info should not be retrieved, as why should we had that info?
	ClientDataHelper *client = new ClientDataHelper();
	client->setProto(P_PROTO);
	client->setName(PNAME);
	client->setPID(ID);
	client->setFullVer(ver);
	client->setPlatform(platform);
	client->setServerObj(this);
	client->setSerializationVer(SRZ_VER);
	client->setTrueSAOId(SAO_ID);
	
	actionstream << "A client are joining into the server: " << std::string(PNAME) << " with id of " << ID << std::endl;
	PlayerSAO *sao = InitClientByMainServer(ID, client);
	
	//s64 last_login;
	//m_script->getAuth(PNAME, nullptr, nullptr, &last_login);
	//m_script->on_joinplayer(sao, last_login);
	QueueOnJoinPlayer(sao, PNAME);
}

//Disconnect player
void Server::handleCommand_GotDisconnect(NetworkPacket* pkt) {
	if (pkt->getSize() < 2) {
		warningstream << "Packet are not complete 'handleCommand_GotDisconnect'" << std::endl;
		return;
	}
	
	u16 PLAYER_ID = 0;
	*pkt >> PLAYER_ID;
	if (PLAYER_ID != 0) {
		DeleteClient(PLAYER_ID, CDR_LEAVE);
	} else {
		errorstream << "Unable to get playerID" << std::endl;
	}
}

void Server::handleCommand_PlayerPosAdvanced(NetworkPacket *pkt) {
	if (pkt->getSize() < 2)
		return;
	
	u16 pcount;
	
	*pkt >> pcount;
	
	for (u16 i = 0; i < pcount; ++i) {
		u16 PLAYER_ID;
		v3s32 POS, SPEED;
		s32 PITCH, YAW;
		u8 WANTEDRANGE, FOV;
		u32 KEYPRESSED;
		bool unv1, unv2, kpg = false;
		*pkt >> PLAYER_ID >> POS >> SPEED >> PITCH >> YAW >> KEYPRESSED >> FOV >> WANTEDRANGE >> kpg >> unv1 >> unv2;
		if (PLAYER_ID == 0) {
			warningstream << "Found player id 0, this might be a bug" << std::endl;
			continue;
		}
		//get player
		RemotePlayer *player = m_env->getPlayer(PLAYER_ID);
		if (!player)
			continue;
		PlayerSAO *sao = player->getPlayerSAO();
		if (!sao)
			continue;
		//transform
		f32 pitch = (f32)PITCH / 100.0f;
		f32 yaw = (f32)YAW / 100.0f;
		v3f position((f32)POS.X / 100.0f, (f32)POS.Y / 100.0f, (f32)POS.Z / 100.0f);
		v3f speed((f32)SPEED.X / 100.0f, (f32)SPEED.Y / 100.0f, (f32)SPEED.Z / 100.0f);
		pitch = modulo360f(pitch);
		yaw = wrapDegrees_0_360(yaw);
		if (!sao->isAttached()) {
			sao->setBasePosition(position);
			player->setSpeed(speed);
		}
		sao->setLookPitch(pitch);
		sao->setPlayerYaw(yaw);
		sao->setFov(FOV);
		sao->setWantedRange(WANTEDRANGE);
		
		bool lft = (KEYPRESSED & (0x1 << 8));
		bool rht = (KEYPRESSED & (0x1 << 7));
		if (
			player->btn_dig != rht ||
			player->btn_place != lft
		) {
			//m_script->PLAYER_on_click(player, rht, lft);
			player->btn_dig   = rht;
			player->btn_place = lft;
		}

		player->keyPressed = KEYPRESSED;
		player->control.up    = (KEYPRESSED & (0x1 << 0));
		player->control.down  = (KEYPRESSED & (0x1 << 1));
		player->control.left  = (KEYPRESSED & (0x1 << 2));
		player->control.right = (KEYPRESSED & (0x1 << 3));
		player->control.jump  = (KEYPRESSED & (0x1 << 4));
		player->control.aux1  = (KEYPRESSED & (0x1 << 5));
		player->control.sneak = (KEYPRESSED & (0x1 << 6));
		player->control.dig   = (KEYPRESSED & (0x1 << 7));
		player->control.place = (KEYPRESSED & (0x1 << 8));
		player->control.zoom  = (KEYPRESSED & (0x1 << 9));

		if (sao->checkMovementCheat()) {
			// Call callbacks
			//m_script->on_cheat(sao, "moved_too_fast");
			SendMovePlayer(PLAYER_ID);
		}
		
		//If not a multicraft player the grab the actions
		if (!player->multicraftplayer) {
			player->sneakpressed = (KEYPRESSED & (0x1 << 6));
		}
		
		if (KEYPRESSED != player->LastKeyPressed) {
			//m_script->PLAYER_on_move(player, KEYPRESSED, pitch);
			player->LastKeyPressed = KEYPRESSED;
		}
		
		if (player->lastPitch != pitch) {
			player->lastPitch = pitch;
			//m_script->PLAYER_head_move(player, pitch);
		}
	}
}

void Server::handleCommand_RequestMedia(NetworkPacket* pkt)
{
	std::vector<std::string> tosend;
	u16 numfiles;

	*pkt >> numfiles;

	session_t peer_id = pkt->getPeerId();
	infostream << "Sending " << numfiles << " files to " <<
		getPlayerName(peer_id) << std::endl;
	verbosestream << "TOSERVER_REQUEST_MEDIA: " << std::endl;

	for (u16 i = 0; i < numfiles; i++) {
		std::string name;

		*pkt >> name;

		tosend.push_back(name);
		verbosestream << "TOSERVER_REQUEST_MEDIA: requested file "
				<< name << std::endl;
	}

	sendRequestedMedia(peer_id, tosend);
}

void Server::handleCommand_ClientReady(NetworkPacket* pkt)
{
	session_t peer_id = pkt->getPeerId();

	if (ServersNetworkObject->AreSlave) {
		errorstream << "Declined handleCommand_Init2(PKT)" << std::endl;
		return;
	}

	PlayerSAO* playersao = StageTwoClientInit(peer_id);
	RemoteClient *client = nullptr; 
	try {
		client = getClient(peer_id, CS_InitDone);
	} catch ( ClientNotFoundException &e ) {
		warningstream << "Failure to get player " << peer_id << std::endl;
		return; // This catches the joining of another player IF the player already joined
	}
	u16 protocol_version = m_clients.getProtocolVersion(peer_id);

	if (playersao == NULL) {
		errorstream << "TOSERVER_CLIENT_READY stage 2 client init failed "
			"peer_id=" << peer_id << std::endl;
		DisconnectPeer(peer_id);
		return;
	}


	if (pkt->getSize() < 8) {
		errorstream << "TOSERVER_CLIENT_READY client sent inconsistent data, "
			"disconnecting peer_id: " << peer_id << std::endl;
		DisconnectPeer(peer_id);
		return;
	}

	// Send active objects
	{
		PlayerSAO *sao = getPlayerSAO(peer_id);
		if (sao)
			SendActiveObjectRemoveAdd(client, sao);
	}

	// Send detached inventories
	sendDetachedInventories(peer_id, false);

	// Send player movement settings
	SendMovement(peer_id, protocol_version);

	// Send time of day
	u16 time = m_env->getTimeOfDay();
	float time_speed = g_settings->getFloat("time_speed");
	SendTimeOfDay(peer_id, time, time_speed);

	SendCSMRestrictionFlags(peer_id);

	u8 major_ver, minor_ver, patch_ver, reserved;
	std::string full_ver;
	*pkt >> major_ver >> minor_ver >> patch_ver >> reserved >> full_ver;

	m_clients.setClientVersion(peer_id, major_ver, minor_ver, patch_ver,
		full_ver);

	if (pkt->getRemainingBytes() >= 2)
		*pkt >> playersao->getPlayer()->formspec_version;

	const std::vector<std::string> &players = m_clients.getPlayerNames();
	NetworkPacket list_pkt(TOCLIENT_UPDATE_PLAYER_LIST, 0, peer_id);
	list_pkt << (u8) PLAYER_LIST_INIT << (u16) players.size();
	for (const std::string &player: players) {
		list_pkt <<  player;
	}
	m_clients.send(peer_id, 0, &list_pkt, true);

	NetworkPacket notice_pkt(TOCLIENT_UPDATE_PLAYER_LIST, 0, PEER_ID_INEXISTENT);
	// (u16) 1 + std::string represents a pseudo vector serialization representation
	notice_pkt << (u8) PLAYER_LIST_ADD << (u16) 1 << std::string(playersao->getPlayer()->getName());
	m_clients.sendToAll(&notice_pkt);
	m_clients.event(peer_id, CSE_SetClientReady);

	s64 last_login;
	//m_script->getAuth(playersao->getPlayer()->getName(), nullptr, nullptr, &last_login);
	//m_script->on_joinplayer(playersao, last_login);
	(reinterpret_cast<void(*)(PlayerSAO*)>(AddonsCallbacks[CALLBACK_ON_JOINPLAYER]))(playersao);

	// Send shutdown timer if shutdown has been scheduled
	if (m_shutdown_state.isTimerRunning()) {
		SendChatMessage(peer_id, m_shutdown_state.getShutdownTimerMessage());
	}
}

struct __hlp {
	content_t org_id = 0;
	content_t new_id = 0;
};

void Server::handleCommand_JumpDefinitions(NetworkPacket *pkt) {
	verbosestream << FUNCTION_NAME << std::endl;
	//For now we will change ONLY the definitions of IDs
	register u16 count = 0;
	*pkt >> count;
	std::unordered_map<content_t, content_t> OriginaltoNew;
	std::unordered_map<content_t, ContentFeatures> to_copy; //Copy definitions so we can unregister they and THEN register in the new allocated ID, idx are original
	for (u16 i = 0; i < count; i++) {
		u16 original_id, new_id = 0;
		*pkt >> original_id >> new_id;
		OriginaltoNew[original_id] = new_id;
		to_copy[original_id] = m_nodedef->get((content_t)original_id);
		verbosestream << FUNCTION_NAME << " > ToCopy: " << to_copy[original_id].name << " | newID: " << new_id << std::endl;
		//unregister the original node id
		//m_nodedef->removeNode(to_copy[original_id].name);
	}
	
	//May deSerialize would work?
	
	//May sort to get from minimun to maximun value
	std::unordered_map<content_t, content_t> to_copy2 = std::move(OriginaltoNew);
	std::deque<content_t> _q;
	register u16 minimun_val = 65535;
	register u16 idx_tmp = 0;
	for (;;) {
		if (to_copy2.size() == 0)
			break;
		for (auto it = to_copy2.begin(); it != to_copy2.end(); ++it) {
			if (it->second < minimun_val) {
				idx_tmp = it->first;
				minimun_val = it->second;
			}
		}
		//We got the minimun value so erase the index
		_q.push_back(idx_tmp);
		//erase
		to_copy2.erase(idx_tmp);
		//reset
		minimun_val = 65535;
		idx_tmp = 0;
	}
	
	verbosestream << FUNCTION_NAME << " > GoingToRegister: " << _q.size() << std::endl;
	
	//Register
	register u16 idx = m_nodedef->getSize();
	while (!_q.empty()) {
		content_t value = _q.front();
		_q.pop_front();
		if (OriginaltoNew[value] > m_nodedef->getSize()) {
			ContentFeatures *f = &to_copy[value];
			if (f->name.empty()) {
				continue;
			}
			bool stop = false;
			while (!stop) {
				idx++;
				if (idx == OriginaltoNew[value])
					break; //Found space for new block
				//Register dummy nodes id so we can make the new id fit in
				std::stringstream ss;
				ss << "dummy_";
				ss << idx;
				m_nodedef->allocateDummy(ss.str());
				verbosestream << FUNCTION_NAME << ">Dummy: " << idx << std::endl;
			}
			m_nodedef->set(to_copy[value].name, to_copy[value]);  //register
		} else { //Directly register
			m_nodedef->set(to_copy[value].name, to_copy[value]);
		}
	}
	
	m_nodedef->setNodeRegistrationStatus(true);
	// Perform pending node name resolutions
	m_nodedef->runNodeResolveCallbacks();
	// unmap node names in cross-references
	m_nodedef->resolveCrossrefs();
	// init the recipe hashes to speed up crafting
	m_craftdef->initHashes(this);
	m_emerge->initMapgens(m_startup_server_map->getMapgenParams());
	
	m_definitions_done = true;
}

void Server::handleCommand_GotBlocks(NetworkPacket* pkt)
{
	if (pkt->getSize() < 1)
		return;

	//Are this executing in a proxy?
	if (!ServersNetworkObject->AreSlave) {
		
		/*
			[0] u16 command
			[2] u8 count
			[3] v3s16 pos_0
			[3+6] v3s16 pos_1
			...
		*/

		u8 count;
		*pkt >> count;

		if ((s16)pkt->getSize() < 1 + (int)count * 6) {
			throw con::InvalidIncomingDataException
					("GOTBLOCKS length is too short");
		}

		m_clients.lock();
		RemoteClient *client = m_clients.lockedGetClientNoEx(pkt->getPeerId());

		for (u16 i = 0; i < count; i++) {
			v3s16 p;
			*pkt >> p;
			client->GotBlock(p);
		}
		m_clients.unlock();
	} else {
		//Then this is a slave
		/*
			[0] u16 command
			[2] u16 PlayerID
			[3] u8 count
			[4] v3s16 pos_0
			[5+6] v3s16 pos_1
			...
		*/
		//playerid
		u16 playerID;
		*pkt >> playerID;
		//count
		u8 count;
		*pkt >> count;
		
		if ((s16)pkt->getSize() < 1 + (int)count * 6) {
			throw con::InvalidIncomingDataException("GOTBLOCKS length is too short");
		}
		
		if (!ClientDataTable.Has(playerID))
			return;
		
		ClientDataHelper *client = ClientDataTable.Get(playerID);
		if (!client)
			return;
		for (u16 i = 0; i < count; i++) {
			v3s16 p;
			*pkt >> p;
			client->GotBlock(p);
		}
	}
}

void Server::process_PlayerPos(RemotePlayer *player, PlayerSAO *playersao,
	NetworkPacket *pkt)
{
	if (pkt->getRemainingBytes() < 12 + 12 + 4 + 4)
		return;

	v3s32 ps, ss;
	s32 f32pitch, f32yaw;
	u8 f32fov;

	*pkt >> ps;
	*pkt >> ss;
	*pkt >> f32pitch;
	*pkt >> f32yaw;

	f32 pitch = (f32)f32pitch / 100.0f;
	f32 yaw = (f32)f32yaw / 100.0f;
	u32 keyPressed = 0;

	// default behavior (in case an old client doesn't send these)
	f32 fov = 0;
	u8 wanted_range = 0;

	if (pkt->getRemainingBytes() >= 4)
		*pkt >> keyPressed;
	if (pkt->getRemainingBytes() >= 1) {
		*pkt >> f32fov;
		fov = (f32)f32fov / 80.0f;
		if (pkt->getRemainingBytes() >= 1)
			*pkt >> wanted_range;
	}

	v3f position((f32)ps.X / 100.0f, (f32)ps.Y / 100.0f, (f32)ps.Z / 100.0f);
	v3f speed((f32)ss.X / 100.0f, (f32)ss.Y / 100.0f, (f32)ss.Z / 100.0f);

	pitch = modulo360f(pitch);
	yaw = wrapDegrees_0_360(yaw);

	if (!playersao->isAttached()) {
		// Only update player positions when moving freely
		// to not interfere with attachment handling
		playersao->setBasePosition(position, !ServersNetworkObject->AreSlave);
		player->setSpeed(speed);
	}
	playersao->setLookPitch(pitch);
	playersao->setPlayerYaw(yaw);
	playersao->setFov(fov);
	playersao->setWantedRange(wanted_range);

	//Should we check at interact
	//if (player->btn_dig != (keyPressed & (0x1 << 7)) || player->btn_place != (keyPressed & (0x1 << 8))) {
	bool lft = (keyPressed & (0x1 << 8));
	bool rht = (keyPressed & (0x1 << 7));
	if (
		player->btn_dig != rht ||
		player->btn_place != lft
	) {
		//m_script->PLAYER_on_click(player, rht, lft);
		player->btn_dig   = rht;
		player->btn_place = lft;
	}

	player->keyPressed = keyPressed;
	player->control.up    = (keyPressed & (0x1 << 0));
	player->control.down  = (keyPressed & (0x1 << 1));
	player->control.left  = (keyPressed & (0x1 << 2));
	player->control.right = (keyPressed & (0x1 << 3));
	player->control.jump  = (keyPressed & (0x1 << 4));
	player->control.aux1  = (keyPressed & (0x1 << 5));
	player->control.sneak = (keyPressed & (0x1 << 6));
	player->control.dig   = (keyPressed & (0x1 << 7));
	player->control.place = (keyPressed & (0x1 << 8));
	player->control.zoom  = (keyPressed & (0x1 << 9));

	//if (playersao->checkMovementCheat()) {
		// Call callbacks
		//m_script->on_cheat(playersao, "moved_too_fast");
		//SendMovePlayer(pkt->getPeerId());
	//}
	
	//If not a multicraft player the grab the actions
	if (!player->multicraftplayer) {
		player->sneakpressed = (keyPressed & (0x1 << 6));
	}
	
	if (keyPressed != player->LastKeyPressed) {
		//m_script->PLAYER_on_move(player, keyPressed, pitch);
		player->LastKeyPressed = keyPressed;
	}
	
	if (player->lastPitch != pitch) {
		player->lastPitch = pitch;
		//m_script->PLAYER_head_move(player, pitch);
	}
}

void Server::handleCommand_PlayerPos(NetworkPacket* pkt)
{
	if (ServersNetworkObject->IsThisAProxy) {
		session_t peer_id = pkt->getPeerId();
		RemotePlayer *player = m_env->getPlayer(peer_id);
		if (player == NULL) {
			errorstream <<
				"Server::ProcessData(): Canceling: No player for peer_id=" <<
				peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		}

		PlayerSAO *playersao = player->getPlayerSAO();
		if (playersao == NULL) {
			errorstream <<
				"Server::ProcessData(): Canceling: No player object for peer_id=" <<
				peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		}

		// If player is dead we don't care of this packet
		if (playersao->isDead()) {
			verbosestream << "TOSERVER_PLAYERPOS: " << player->getName()
					<< " is dead. Ignoring packet";
			return;
		}

		process_PlayerPos(player, playersao, pkt);
	} else {
		//Acquire PlayerID
		u16 pid;
		*pkt >> pid;
		RemotePlayer *player = m_env->getPlayer(pid);
		if (player == NULL) {
			errorstream << "Server::ProcessData(): Canceling: No player for pid=" << pid << " disconnecting peer!" << std::endl;
			return;
		}
		PlayerSAO *playersao = player->getPlayerSAO();
		if (playersao == NULL) {
			errorstream <<
				"Server::ProcessData(): Canceling: No player object for pid=" << pid << " disconnecting peer!" << std::endl;
			return;
		}
		// If player is dead we don't care of this packet
		if (playersao->isDead()) {
			verbosestream << "TOSERVER_PLAYERPOS: " << player->getName() << " is dead. Ignoring packet";
			return;
		}
		process_PlayerPos(player, playersao, pkt);
	}
}

void Server::handleCommand_DeletedBlocks(NetworkPacket* pkt)
{
	if (pkt->getSize() < 1)
		return;

	//Are this executing in a proxy?
	if (ServersNetworkObject->IsThisAProxy) {
		
		/*
			[0] u16 command
			[2] u8 count
			[3] v3s16 pos_0
			[3+6] v3s16 pos_1
			...
		*/
		u8 count;
		*pkt >> count;
		if ((s16)pkt->getSize() < 1 + (int)count * 6) {
			throw con::InvalidIncomingDataException
					("DELETEDBLOCKS length is too short");
		}

		RemoteClient *client = m_clients.lockedGetClientNoEx(pkt->getPeerId());

		for (u16 i = 0; i < count; i++) {
			v3s16 p;
			*pkt >> p;
			client->SetBlockNotSent(p);
		}
	} else {
		//Then this is a slave
		/*
			[0] u16 command
			[2] u16 PlayerID
			[3] u8 count
			[4] v3s16 pos_0
			[5+6] v3s16 pos_1
			...
		*/
		//playerid
		u16 playerID;
		*pkt >> playerID;
		//count
		u8 count;
		*pkt >> count;
		
		if ((s16)pkt->getSize() < 1 + (int)count * 6) {
			throw con::InvalidIncomingDataException("DELETEDBLOCKS length is too short");
		}
		
		if (!ClientDataTable.Has(playerID))
			return;
		
		ClientDataHelper *client = ClientDataTable.Get(playerID);
		if (!client)
			return;
		for (u16 i = 0; i < count; i++) {
			v3s16 p;
			*pkt >> p;
			client->SetBlockNotSent(p);
		}
	}
}

void Server::handleCommand_InventoryAction(NetworkPacket* pkt)
{
	RemotePlayer *player;
	size_t to_sum = 0;
	u8 index__ = 0;
	u16 playerid = 0;
	session_t peer_id = pkt->getPeerId();
	if (ServersNetworkObject->IsThisAProxy) {
		player = m_env->getPlayer(peer_id);
		if (player == NULL) {
			errorstream << "Server::ProcessData(): Canceling: No player for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		}
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
		player = m_env->getPlayer(playerid);
		to_sum = 2; //remove 2 bytes as it are the player id
		index__ = 2; //add 2 indexes [u16, LongString] = {PlayerID, std::osstring}
	}
	
	//player = m_env->getPlayer(peer_id);

	

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player object for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player object for peer_id=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}

	// Strip command and create a stream
	std::string datastring(pkt->getString(0+index__), pkt->getSize()-to_sum);
	verbosestream << "TOSERVER_INVENTORY_ACTION: data=" << datastring
		<< std::endl;
	std::istringstream is(datastring, std::ios_base::binary);
	// Create an action
	std::unique_ptr<InventoryAction> a(InventoryAction::deSerialize(is));
	if (!a) {
		infostream << "TOSERVER_INVENTORY_ACTION: "
				<< "InventoryAction::deSerialize() returned NULL"
				<< std::endl;
		return;
	}

	/*
		Note: Always set inventory not sent, to repair cases
		where the client made a bad prediction.
	*/

	const bool player_has_interact = checkPriv(player->getName(), "interact");

	auto check_inv_access = [player, player_has_interact, this] (
			const InventoryLocation &loc) -> bool {

		// Players without interact may modify their own inventory
		if (!player_has_interact && loc.type != InventoryLocation::PLAYER) {
			infostream << "Cannot modify foreign inventory: "
					<< "No interact privilege" << std::endl;
			return false;
		}

		switch (loc.type) {
		case InventoryLocation::CURRENT_PLAYER:
			// Only used internally on the client, never sent
			return false;
		case InventoryLocation::PLAYER:
			// Allow access to own inventory in all cases
			return loc.name == player->getName();
		case InventoryLocation::NODEMETA:
			{
				// Check for out-of-range interaction
				v3f node_pos   = intToFloat(loc.p, BS);
				v3f player_pos = player->getPlayerSAO()->getEyePosition();
				f32 d = player_pos.getDistanceFrom(node_pos);
				return checkInteractDistance(player, d, "inventory");
			}
		case InventoryLocation::DETACHED:
			return getInventoryMgr()->checkDetachedInventoryAccess(loc, player->getName());
		default:
			return false;
		}
	};

	/*
		Handle restrictions and special cases of the move action
	*/
	if (a->getType() == IAction::Move) {
		IMoveAction *ma = (IMoveAction*)a.get();

		ma->from_inv.applyCurrentPlayer(player->getName());
		ma->to_inv.applyCurrentPlayer(player->getName());

		m_inventory_mgr->setInventoryModified(ma->from_inv);
		if (ma->from_inv != ma->to_inv)
			m_inventory_mgr->setInventoryModified(ma->to_inv);

		if (!check_inv_access(ma->from_inv) ||
				!check_inv_access(ma->to_inv))
			return;

		/*
			Disable moving items out of craftpreview
		*/
		if (ma->from_list == "craftpreview") {
			infostream << "Ignoring IMoveAction from "
					<< (ma->from_inv.dump()) << ":" << ma->from_list
					<< " to " << (ma->to_inv.dump()) << ":" << ma->to_list
					<< " because src is " << ma->from_list << std::endl;
			return;
		}

		/*
			Disable moving items into craftresult and craftpreview
		*/
		if (ma->to_list == "craftpreview" || ma->to_list == "craftresult") {
			infostream << "Ignoring IMoveAction from "
					<< (ma->from_inv.dump()) << ":" << ma->from_list
					<< " to " << (ma->to_inv.dump()) << ":" << ma->to_list
					<< " because dst is " << ma->to_list << std::endl;
			return;
		}
	}
	/*
		Handle restrictions and special cases of the drop action
	*/
	else if (a->getType() == IAction::Drop) {
		IDropAction *da = (IDropAction*)a.get();

		da->from_inv.applyCurrentPlayer(player->getName());

		m_inventory_mgr->setInventoryModified(da->from_inv);

		/*
			Disable dropping items out of craftpreview
		*/
		if (da->from_list == "craftpreview") {
			infostream << "Ignoring IDropAction from "
					<< (da->from_inv.dump()) << ":" << da->from_list
					<< " because src is " << da->from_list << std::endl;
			return;
		}

		// Disallow dropping items if not allowed to interact
		if (!player_has_interact || !check_inv_access(da->from_inv))
			return;

		// Disallow dropping items if dead
		if (playersao->isDead()) {
			infostream << "Ignoring IDropAction from "
					<< (da->from_inv.dump()) << ":" << da->from_list
					<< " because player is dead." << std::endl;
			return;
		}
	}
	/*
		Handle restrictions and special cases of the craft action
	*/
	else if (a->getType() == IAction::Craft) {
		ICraftAction *ca = (ICraftAction*)a.get();

		ca->craft_inv.applyCurrentPlayer(player->getName());

		m_inventory_mgr->setInventoryModified(ca->craft_inv);

		// Disallow crafting if not allowed to interact
		if (!player_has_interact) {
			infostream << "Cannot craft: "
					<< "No interact privilege" << std::endl;
			return;
		}

		if (!check_inv_access(ca->craft_inv))
			return;
	} else {
		// Unknown action. Ignored.
		return;
	}

	// Do the action
	a->apply(m_inventory_mgr.get(), playersao, this);
}

void Server::handleCommand_ChatMessage(NetworkPacket* pkt)
{
	if (ServersNetworkObject->AreSlave) {
		u16 PID;
		std::wstring message;
		*pkt >> PID;
		*pkt >> message;
		RemotePlayer *player = m_env->getPlayer(PID);
		if (player == NULL) {
			errorstream << "Server::ProcessData(): Canceling: No player for player_id=" << PID << " disconnecting peer!" << std::endl;
			DeletePlayer(PID);
			return;
		}
		std::string name = player->getName();
		std::wstring answer_to_sender = handleChat(name, message, true, player);
		if (!answer_to_sender.empty()) {
			// Send the answer to sender
			SendChatMessage(PID, ChatMessage(CHATMESSAGE_TYPE_SYSTEM, answer_to_sender));
		}
	} else {
		std::wstring message;
		*pkt >> message;
		session_t peer_id = pkt->getPeerId();
		RemotePlayer *player = m_env->getPlayer(peer_id);
		if (player == NULL) {
			errorstream << "Server::ProcessData(): Canceling: No player for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		}
		std::string name = player->getName();
		std::wstring answer_to_sender = handleChat(name, message, true, player);
		if (!answer_to_sender.empty()) {
			// Send the answer to sender
			SendChatMessage(peer_id, ChatMessage(CHATMESSAGE_TYPE_SYSTEM, answer_to_sender));
		}
	}
}

void Server::handleCommand_Damage(NetworkPacket* pkt)
{

	RemotePlayer *player;
	session_t peer_id = pkt->getPeerId();
	u16 playerid;
	if (ServersNetworkObject->IsThisAProxy) {
		player = m_env->getPlayer(peer_id);
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
		player = m_env->getPlayer(playerid);
	}

	if (ServersNetworkObject->IsThisAProxy) {
		if (player == NULL) {
			errorstream <<
				"Server::ProcessData(): Canceling: No player for peer_id=" <<
				peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		}
	} else {
		if (player == NULL) {
			errorstream << "Server::ProcessData(): Canceling: No player for pid=" << playerid << " disconnecting player!" << std::endl;
			DeletePlayer(playerid);
			return;
		}
	}

	u16 damage;

	//if (ServersNetworkObject->IsThisAProxy) {
		// Minetest 0.4 uses 8-bit integers for damage.
		if (player->protocol_version >= 37) {
			*pkt >> damage;
		} else {
			u8 raw_damage;
			*pkt >> raw_damage;
			damage = raw_damage;
		}
	//} else {
		//Sent-By-Server always return a uint16_t value
	//	*pkt >> damage;
	//}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		if (!ServersNetworkObject->AreSlave) {
			errorstream <<
				"Server::ProcessData(): Canceling: No player object for peer_id=" <<
				peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player for pid=" << playerid << " disconnecting player!" << std::endl;
			DeletePlayer(playerid);
			return;
		}
	}

	if (!playersao->isImmortal()) {
		if (playersao->isDead()) {
			verbosestream << "Server::ProcessData(): Info: "
				"Ignoring damage as player " << player->getName()
				<< " is already dead." << std::endl;
			return;
		}

		actionstream << player->getName() << " damaged by "
				<< (int)damage << " hp at " << PP(playersao->getBasePosition() / BS)
				<< std::endl;

		PlayerHPChangeReason reason(PlayerHPChangeReason::FALL);
		playersao->setHP((s32)playersao->getHP() - (s32)damage, reason);
		SendPlayerHPOrDie(playersao, reason);
	}
}

//POSSIBLE EXPLOIT IS IN HERE!
void Server::handleCommand_PlayerItem(NetworkPacket* pkt)
{
	if (pkt->getSize() < 2)
		return;

	RemotePlayer *player;
	session_t peer_id;
	u16 playerid;
	if (ServersNetworkObject->IsThisAProxy) {
		peer_id = pkt->getPeerId();
		player = m_env->getPlayer(peer_id);
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
		player = m_env->getPlayer(playerid);
	}

	if (player == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player obj for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player obj for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}

	u16 item;

	*pkt >> item;

	playersao->getPlayer()->setWieldIndex(item);
}

void Server::handleCommand_Respawn(NetworkPacket* pkt)
{
	RemotePlayer *player;
	session_t peer_id;
	u16 playerid;
	if (ServersNetworkObject->IsThisAProxy) {
		peer_id = pkt->getPeerId();
		player = m_env->getPlayer(peer_id);
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
		player = m_env->getPlayer(playerid);
	}
	
	if (player == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}
	
	PlayerSAO *playersao = player->getPlayerSAO();
	assert(playersao);

	if (!playersao->isDead())
		return;

	RespawnPlayer(ServersNetworkObject->AreSlave ? playerid : peer_id);

	actionstream << player->getName() << " respawns at " << PP(playersao->getBasePosition() / BS) << std::endl;

	// ActiveObject is added to environment in AsyncRunStep after
	// the previous addition has been successfully removed
}

bool Server::checkInteractDistance(RemotePlayer *player, const f32 d, const std::string &what)
{
	ItemStack selected_item, hand_item;
	player->getWieldedItem(&selected_item, &hand_item);
	f32 max_d = BS * getToolRange(selected_item.getDefinition(m_itemdef),
			hand_item.getDefinition(m_itemdef));

	// Cube diagonal * 1.5 for maximal supported node extents:
	// sqrt(3) * 1.5 ≅ 2.6
	if (d > max_d + 2.6f * BS) {
		actionstream << "Player " << player->getName()
				<< " tried to access " << what
				<< " from too far: "
				<< "d=" << d << ", max_d=" << max_d
				<< "; ignoring." << std::endl;
		// Call callbacks
		//m_script->on_cheat(player->getPlayerSAO(), "interacted_too_far");
		return false;
	}
	return true;
}

void Server::handleCommand_Interact(NetworkPacket *pkt)
{
	/*
		[0] u16 command
		[?] u16 player id
		[2] u8 action
		[3] u16 item
		[5] u32 length of the next item (plen)
		[9] serialized PointedThing
		[9 + plen] player position information
	*/

	RemotePlayer *player;
	u16 playerid;
	session_t peer_id;
	if (ServersNetworkObject->IsThisAProxy) {
		peer_id = pkt->getPeerId();
		player = m_env->getPlayer(peer_id);
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
		player = m_env->getPlayer(playerid);
	}

	if (!ServersNetworkObject->AreSlave)
		playerid = player->getPlayerID(); //Useful for map matching

	PlayerSAO *playersao;

	if (ServersNetworkObject->IsThisAProxy) {
		if (player == NULL) {
			errorstream <<
				"Server::ProcessData(): Canceling: No player for peer_id=" <<
				peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		}
		playersao = player->getPlayerSAO();
		if (playersao == NULL) {
			errorstream <<
				"Server::ProcessData(): Canceling: No player object for peer_id=" <<
				peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
			return;
		}
	} else {
		if (player == NULL) {
			errorstream << "Server::ProcessData(): Canceling: No player!.. disconnecting unknown ID for " << playerid << std::endl;
			DeletePlayer(playerid);
			return;
		}
		playersao = player->getPlayerSAO();
		if (playersao == NULL) {
			errorstream << "Server::ProcessData(): Canceling: No player object for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
			return;
		}
	}

	InteractAction action;
	u16 item_i;

	*pkt >> (u8 &)action;
	*pkt >> item_i;

	std::istringstream tmp_is(pkt->readLongString(), std::ios::binary);
	PointedThing pointed;
	pointed.deSerialize(tmp_is);

	verbosestream << "TOSERVER_INTERACT: action=" << (int)action << ", item=" << item_i << ", pointed=" << pointed.dump() << std::endl;

	if (playersao->isDead()) {
		actionstream << "Server: " << player->getName() << " tried to interact while dead; ignoring." << std::endl;
		if (pointed.type == POINTEDTHING_NODE) {
			// Re-send block to revert change on client-side
			//RemoteClient *client = getClient(peer_id);
			if (ServersNetworkObject->AreSlave) {
				ClientDataHelper *client = ClientDataTable.Get(playerid);
				v3s16 blockpos = getNodeBlockPos(pointed.node_undersurface);
				client->SetBlockNotSent(blockpos);
			} else {
				RemoteClient *client = getClient(peer_id);
				v3s16 blockpos = getNodeBlockPos(pointed.node_undersurface);
				client->SetBlockNotSent(blockpos);
			}
		}
		// Call callbacks
		//m_script->on_cheat(playersao, "interacted_while_dead");
		(reinterpret_cast<void(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_CHEAT]))(playersao, "interacted_while_dead");
		return;
	}

	process_PlayerPos(player, playersao, pkt);

	v3f player_pos = playersao->getLastGoodPosition();

	// Update wielded item
	playersao->getPlayer()->setWieldIndex(item_i);

	// Get pointed to object (NULL if not POINTEDTYPE_OBJECT)
	ServerActiveObject *pointed_object = NULL;
	if (pointed.type == POINTEDTHING_OBJECT) {
		if (!ServersNetworkObject->AreSlave) {
			pointed_object = m_env->getActiveObject(pointed.object_id);
		} else {
			pointed_object = m_env->getActiveObject(0); //TODO: Must make a reflect from the proxy to here'
		}
		if (pointed_object == NULL) {
			verbosestream << "TOSERVER_INTERACT: "
				"pointed object is NULL" << std::endl;
			return;
		}

	}

	/*
		Make sure the player is allowed to do it
	*/
	if (!checkPriv(player->getName(), "interact")) {
		actionstream << player->getName() << " attempted to interact with " <<
				pointed.dump() << " without 'interact' privilege" << std::endl;

		if (pointed.type != POINTEDTHING_NODE)
			return;

		// Re-send block to revert change on client-side
		if (ServersNetworkObject->AreSlave) {
			ClientDataHelper *client = ClientDataTable.Get(playerid);
			// Digging completed -> under
			if (action == INTERACT_DIGGING_COMPLETED) {
				v3s16 blockpos = getNodeBlockPos(pointed.node_undersurface);
				client->SetBlockNotSent(blockpos);
			}
			// Placement -> above
			else if (action == INTERACT_PLACE) {
				v3s16 blockpos = getNodeBlockPos(pointed.node_abovesurface);
				client->SetBlockNotSent(blockpos);
			}
		} else {
			RemoteClient *client = getClient(peer_id);
			// Digging completed -> under
			if (action == INTERACT_DIGGING_COMPLETED) {
				v3s16 blockpos = getNodeBlockPos(pointed.node_undersurface);
				client->SetBlockNotSent(blockpos);
			}
			// Placement -> above
			else if (action == INTERACT_PLACE) {
				v3s16 blockpos = getNodeBlockPos(pointed.node_abovesurface);
				client->SetBlockNotSent(blockpos);
			}
		}
		return;
	}

	/*
		Check that target is reasonably close
	*/
	static thread_local const bool enable_anticheat = !g_settings->getBool("disable_anticheat");

	if ((action == INTERACT_START_DIGGING || action == INTERACT_DIGGING_COMPLETED || action == INTERACT_PLACE || action == INTERACT_USE) && enable_anticheat && !isSingleplayer()) {
		if (!ServersNetworkObject->AreSlave) {
			v3f target_pos = player_pos;
			if (pointed.type == POINTEDTHING_NODE) {
				target_pos = intToFloat(pointed.node_undersurface, BS);
			} else if (pointed.type == POINTEDTHING_OBJECT) {
				if (playersao->getId() == pointed_object->getId()) {
					actionstream << "Server: " << player->getName() << " attempted to interact with themselves" << std::endl;
					//m_script->on_cheat(playersao, "interacted_with_self");
					(reinterpret_cast<void(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_CHEAT]))(playersao, "interacted_with_self");
					return;
				}
				target_pos = pointed_object->getBasePosition();
			}
			float d = playersao->getEyePosition().getDistanceFrom(target_pos);

			if (!checkInteractDistance(player, d, pointed.dump())) {
				if (pointed.type == POINTEDTHING_NODE) {
					// Re-send block to revert change on client-side
					RemoteClient *client = getClient(peer_id);
					v3s16 blockpos = getNodeBlockPos(pointed.node_undersurface);
					client->SetBlockNotSent(blockpos);
				}
				return;
			}
		} else {
			v3f target_pos = player_pos;
			if (pointed.type == POINTEDTHING_NODE) {
				target_pos = intToFloat(pointed.node_undersurface, BS);
			} else if (pointed.type == POINTEDTHING_OBJECT) {
				if (playersao->getId() == pointed_object->getId()) {
					actionstream << "Server: " << player->getName() << " attempted to interact with themselves" << std::endl;
					//m_script->on_cheat(playersao, "interacted_with_self");
					(reinterpret_cast<void(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_CHEAT]))(playersao, "interacted_with_self");
					return;
				}
				target_pos = pointed_object->getBasePosition();
			}
			float d = playersao->getEyePosition().getDistanceFrom(target_pos);
			if (!checkInteractDistance(player, d, pointed.dump())) {
				if (pointed.type == POINTEDTHING_NODE) {
					// Re-send block to revert change on client-side
					ClientDataHelper *client = ClientDataTable.Get(playerid);
					v3s16 blockpos = getNodeBlockPos(pointed.node_undersurface);
					client->SetBlockNotSent(blockpos);
				}
				return;
			}
		}
	}

	/*
		If something goes wrong, this player is to blame
	*/

	switch (action) {
	// Start digging or punch object
	case INTERACT_START_DIGGING: {
		ItemStack selected_item, hand_item;
		ItemStack tool_item = playersao->getWieldedItem(&selected_item, &hand_item);
		if (pointed.type == POINTEDTHING_NODE) {
			MapNode n(CONTENT_IGNORE);
			bool pos_ok;

			v3s16 p_under = pointed.node_undersurface;

			if (!playersao)
				return;
			
			n = m_env->getMap(playersao->getMapId()).getNode(p_under, &pos_ok);

			if (!pos_ok) {
				infostream << "Server: Not punching: Node not found. "
					"Adding block to emerge queue." << std::endl;
				Maps.at(PlayerToMap.Get(m_env->getPlayer(playerid)->getName()).mapid)->m_emerge->enqueueBlockEmerge(peer_id,
getNodeBlockPos(pointed.node_abovesurface), false);
			}

			//if (n.getContent() != CONTENT_IGNORE)
			//	(reinterpret_cast<void(*)(v3s16, MapNode, PlayerSAO*, PointedThing)>(AddonsCallbacks[CALLBACK_NODE_ONPUNCH]))(p_under, n, playersao, pointed);
			
			_callback_ACT1_B oP = getNodeDefManager()->get(n.getContent()).on_punch;
			bool _res = false;
			if (oP != nullptr) {
				_res = oP(tool_item, playersao, pointed);
			}
				
				//m_script->node_on_punch(p_under, n, playersao, pointed);

			// Cheat prevention
			playersao->noCheatDigStart(p_under);
			
			if (_res)
				if (selected_item.name != "" && m_itemdef->get(selected_item.name).type == ITEM_TOOL)
					playersao->setWieldedItem(selected_item);
			return;
		}

		// Skip if the object can't be interacted with anymore
		if (pointed.type != POINTEDTHING_OBJECT || pointed_object->isGone())
			return;

		
		ToolCapabilities toolcap = tool_item.getToolCapabilities(m_itemdef);
		v3f dir = (pointed_object->getBasePosition() - (playersao->getBasePosition() + playersao->getEyeOffset())).normalize();
		float time_from_last_punch =playersao->resetTimeFromLastPunch();

		u16 src_original_hp = pointed_object->getHP();
		u16 dst_origin_hp = playersao->getHP();

		u16 wear = pointed_object->punch(dir, &toolcap, playersao, time_from_last_punch);

		// Callback may have changed item, so get it again
		playersao->getWieldedItem(&selected_item);
		bool changed = selected_item.addWear(wear, m_itemdef);
		if (changed)
			playersao->setWieldedItem(selected_item);

		// If the object is a player and its HP changed
		if (src_original_hp != pointed_object->getHP() && pointed_object->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			SendPlayerHPOrDie((PlayerSAO *)pointed_object, PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, playersao));
		}

		// If the puncher is a player and its HP changed
		if (dst_origin_hp != playersao->getHP())
			SendPlayerHPOrDie(playersao, PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, pointed_object));

		return;
	} // action == INTERACT_START_DIGGING

	case INTERACT_STOP_DIGGING:
		// Nothing to do
		return;

	case INTERACT_DIGGING_COMPLETED: {
		// Only digging of nodes
		if (pointed.type != POINTEDTHING_NODE)
			return;
		bool pos_ok;
		v3s16 p_under = pointed.node_undersurface;
		MapNode n = m_env->getMap(PlayerToMap.Get(m_env->getPlayer(playerid)->getName()).mapid).getNode(p_under, &pos_ok);
		if (!pos_ok) {
			infostream << "Server: Not finishing digging: Node not found. "
				"Adding block to emerge queue." << std::endl;
			Maps.at(PlayerToMap.Get(m_env->getPlayer(playerid)->getName()).mapid)->m_emerge->enqueueBlockEmerge(peer_id,
				getNodeBlockPos(pointed.node_abovesurface), false);
		}

		/* Cheat prevention */
		bool is_valid_dig = true;
		if (enable_anticheat && !isSingleplayer()) {
			v3s16 nocheat_p = playersao->getNoCheatDigPos();
			float nocheat_t = playersao->getNoCheatDigTime();
			playersao->noCheatDigEnd();
			// If player didn't start digging this, ignore dig
			if (nocheat_p != p_under) {
				infostream << "Server: " << player->getName()
						<< " started digging "
						<< PP(nocheat_p) << " and completed digging "
						<< PP(p_under) << "; not digging." << std::endl;
				is_valid_dig = false;
				// Call callbacks
				(reinterpret_cast<void(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_CHEAT]))(playersao, "finished_unknown_dig");
				//m_script->on_cheat(playersao, "finished_unknown_dig");
			}

			// Get player's wielded item
			// See also: Game::handleDigging
			ItemStack selected_item, hand_item;
			ItemStack tool = playersao->getPlayer()->getWieldedItem(&selected_item, &hand_item);

			// Get diggability and expected digging time
			DigParams params = getDigParams(m_nodedef->get(n).groups,
					&selected_item.getToolCapabilities(m_itemdef));
			// If can't dig, try hand
			if (!params.diggable) {
				params = getDigParams(m_nodedef->get(n).groups,
					&hand_item.getToolCapabilities(m_itemdef));
			}
			// If can't dig, ignore dig
			if (!params.diggable) {
				infostream << "Server: " << player->getName()
						<< " completed digging " << PP(p_under)
						<< ", which is not diggable with tool; not digging."
						<< std::endl;
				is_valid_dig = false;
				// Call callbacks
				(reinterpret_cast<void(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_CHEAT]))(playersao, "dug_unbreakable");
				//m_script->on_cheat(playersao, "dug_unbreakable");
			}
			// Check digging time
			// If already invalidated, we don't have to
			if (!is_valid_dig) {
				// Well not our problem then
			}
			// Clean and long dig
			else if (params.time > 2.0 && nocheat_t * 1.2 > params.time) {
				// All is good, but grab time from pool; don't care if
				// it's actually available
				playersao->getDigPool().grab(params.time);
			}
			// Short or laggy dig
			// Try getting the time from pool
			else if (playersao->getDigPool().grab(params.time)) {
				// All is good
			}
			// Dig not possible
			else {
				infostream << "Server: " << player->getName()
						<< " completed digging " << PP(p_under)
						<< "too fast; not digging." << std::endl;
				is_valid_dig = false;
				// Call callbacks
				(reinterpret_cast<void(*)(PlayerSAO*, const char*)>(AddonsCallbacks[CALLBACK_ON_CHEAT]))(playersao, "dug_too_fast");
				//m_script->on_cheat(playersao, "dug_too_fast");
			}
		}

		/* Actually dig node */

		ItemStack trash;
		ItemStack tool = playersao->getPlayer()->getWieldedItem(&trash,nullptr);
		if (is_valid_dig && n.getContent() != CONTENT_IGNORE) {
			_callback_ACT1_B cbkl = m_nodedef->get(n.getContent()).on_dig;
			if (cbkl && cbkl(tool, playersao, pointed))
				(reinterpret_cast<void(*)(v3s16, MapNode, PlayerSAO*)>(AddonsCallbacks[CALLBACK_ON_DIG]))(p_under, n, playersao);
		}
			
			//m_script->node_on_dig(p_under, n, playersao);

		v3s16 blockpos = getNodeBlockPos(p_under);
		if (!ServersNetworkObject->AreSlave) {
			RemoteClient *client = getClient(peer_id);
			// Send unusual result (that is, node not being removed)
			if (m_env->getMap(PlayerToMap.Get(m_env->getPlayer(playerid)->getName()).mapid).getNode(p_under).getContent() != CONTENT_AIR)
				// Re-send block to revert change on client-side
				client->SetBlockNotSent(blockpos);
			else
				client->ResendBlockIfOnWire(blockpos);
		} else {
			ClientDataHelper *client = ClientDataTable.Get(playerid);
			// Send unusual result (that is, node not being removed)
			if (m_env->getMap(PlayerToMap.Get(m_env->getPlayer(playerid)->getName()).mapid).getNode(p_under).getContent() != CONTENT_AIR)
				// Re-send block to revert change on client-side
				client->SetBlockNotSent(blockpos);
			else
				client->ResendBlockIfOnWire(blockpos);
		}
		return;
	} // action == INTERACT_DIGGING_COMPLETED

	// Place block or right-click object
	case INTERACT_PLACE: { // Maybe callbacks will be obsolete in this season.
		ItemStack selected_item;
		playersao->getWieldedItem(&selected_item, nullptr);

		// Reset build time counter
		if (pointed.type == POINTEDTHING_NODE && selected_item.getDefinition(m_itemdef).type == ITEM_NODE) {
			if (!ServersNetworkObject->AreSlave)
				getClient(peer_id)->m_time_from_building = 0.0;
			else
				ClientDataTable.Get(playerid)->m_time_from_building = 0.0f;
		}

		(reinterpret_cast<bool(*)(ItemStack, PlayerSAO*, PointedThing)>(AddonsCallbacks[CALLBACK_ON_PLACE]))(selected_item, playersao, pointed);
		
		// Seek main func.
		_callback_ACT1 cblkOP = getNodeDefManager()->get(selected_item.name).on_place;
		
		if (pointed.type == POINTEDTHING_OBJECT) {
		} else if (cblkOP != nullptr) {
		//} else if () {
			cblkOP(selected_item, playersao, pointed);

			// Apply returned ItemStack
			if (playersao->setWieldedItem(selected_item))
				SendInventory(playersao, true);
		}

		if (pointed.type != POINTEDTHING_NODE)
			return;

		// If item has node placement prediction, always send the
		// blocks to make sure the client knows what exactly happened
		if (!ServersNetworkObject->AreSlave) {
			RemoteClient *client = getClient(peer_id);
			v3s16 blockpos = getNodeBlockPos(pointed.node_abovesurface);
			v3s16 blockpos2 = getNodeBlockPos(pointed.node_undersurface);
			if (!selected_item.getDefinition(m_itemdef).node_placement_prediction.empty()) {
				client->SetBlockNotSent(blockpos);
				if (blockpos2 != blockpos)
					client->SetBlockNotSent(blockpos2);
			} else {
				client->ResendBlockIfOnWire(blockpos);
				if (blockpos2 != blockpos)
					client->ResendBlockIfOnWire(blockpos2);
			}
		} else {
			ClientDataHelper *client = ClientDataTable.Get(playerid);
			v3s16 blockpos = getNodeBlockPos(pointed.node_abovesurface);
			v3s16 blockpos2 = getNodeBlockPos(pointed.node_undersurface);
			if (!selected_item.getDefinition(m_itemdef).node_placement_prediction.empty()) {
				client->SetBlockNotSent(blockpos);
				if (blockpos2 != blockpos)
					client->SetBlockNotSent(blockpos2);
			} else {
				client->ResendBlockIfOnWire(blockpos);
				if (blockpos2 != blockpos)
					client->ResendBlockIfOnWire(blockpos2);
			}
		}

		return;
	} // action == INTERACT_PLACE

	case INTERACT_USE: {
		ItemStack selected_item;
		playersao->getWieldedItem(&selected_item, nullptr);

		actionstream << player->getName() << " uses " << selected_item.name
				<< ", pointing at " << pointed.dump() << std::endl;

		//if (m_script->item_OnUse(selected_item, playersao, pointed)) {
		if ((reinterpret_cast<bool(*)(ItemStack, PlayerSAO*, PointedThing)>(AddonsCallbacks[CALLBACK_ON_USE]))(selected_item, playersao, pointed)) {
			// Apply returned ItemStack
			if (playersao->setWieldedItem(selected_item))
				SendInventory(playersao, true);
		}

		return;
	}

	// Rightclick air
	case INTERACT_ACTIVATE: {
		ItemStack selected_item;
		playersao->getWieldedItem(&selected_item, nullptr);

		actionstream << player->getName() << " activates "
				<< selected_item.name << std::endl;

		pointed.type = POINTEDTHING_NOTHING; // can only ever be NOTHING

		if ((reinterpret_cast<bool(*)(ItemStack, PlayerSAO*, PointedThing)>(AddonsCallbacks[CALLBACK_ONSECONDARYUSE]))(selected_item, playersao, pointed)) {
			if (playersao->setWieldedItem(selected_item))
				SendInventory(playersao, true);
		}

		return;
	}

	default:
		warningstream << "Server: Invalid action " << action << std::endl;

	}
}

void Server::handleCommand_RemovedSounds(NetworkPacket* pkt)
{
	//pkt->getPeerId()
	u16 playerid;
	session_t PEERID;
	if (ServersNetworkObject->IsThisAProxy) {
		PEERID = pkt->getPeerId();
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
	}
	
	u16 num;
	*pkt >> num;
	for (u16 k = 0; k < num; k++) {
		s32 id;

		*pkt >> id;

		std::unordered_map<s32, ServerPlayingSound>::iterator i =
			m_playing_sounds.find(id);
		if (i == m_playing_sounds.end())
			continue;

		ServerPlayingSound &psound = i->second;
		if (!ServersNetworkObject->IsThisAProxy)
			psound.clients.erase(PEERID);
		else
			psound.clients_int16.erase(playerid); //need to set up some parts for players engine in sound system
		if (psound.clients.empty())
			m_playing_sounds.erase(i++);
	}
}

static bool pkt_read_formspec_fields(NetworkPacket *pkt, StringMap &fields)
{
	u16 field_count;
	*pkt >> field_count;

	u64 length = 0;
	for (u16 k = 0; k < field_count; k++) {
		std::string fieldname;
		*pkt >> fieldname;
		fields[fieldname] = pkt->readLongString();

		length += fieldname.size();
		length += fields[fieldname].size();
	}
	// 640K ought to be enough for anyone
	return length < 640 * 1024;
}

void Server::handleCommand_NodeMetaFields(NetworkPacket* pkt)
{
	//session_t peer_id = pkt->getPeerId();
	//RemotePlayer *player = m_env->getPlayer(peer_id);
	RemotePlayer *player;
	u16 playerid;
	session_t peer_id;
	if (ServersNetworkObject->IsThisAProxy) {
		peer_id = pkt->getPeerId();
		player = m_env->getPlayer(peer_id);
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
		player = m_env->getPlayer(playerid);
	}

	if (player == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player obj for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player obj for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}

	v3s16 p;
	std::string formname;
	StringMap fields;

	*pkt >> p >> formname;

	if (!pkt_read_formspec_fields(pkt, fields)) {
		warningstream << "Too large formspec fields! Ignoring for pos="
			<< PP(p) << ", player=" << player->getName() << std::endl;
		return;
	}


	//m_script->node_on_receive_fields(p, formname, fields, playersao);
	(reinterpret_cast<void(*)(v3s16, std::string, StringMap, PlayerSAO*)>(AddonsCallbacks[CALLBACK_ON_NODE_RECEIVEFIELDS]))(p, formname, fields, playersao);
}

void Server::handleCommand_InventoryFields(NetworkPacket* pkt)
{
	RemotePlayer *player;
	u16 playerid;
	session_t peer_id;
	if (ServersNetworkObject->IsThisAProxy) {
		peer_id = pkt->getPeerId();
		player = m_env->getPlayer(peer_id);
	} else { //IF ELSE then this is a slave
		*pkt >> playerid;
		player = m_env->getPlayer(playerid);
	}

	if (player == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		if (ServersNetworkObject->IsThisAProxy) {
			errorstream << "Server::ProcessData(): Canceling: No player obj for peer_id=" << peer_id << " disconnecting peer!" << std::endl;
			DisconnectPeer(peer_id);
		} else {
			errorstream << "Server::ProcessData(): Canceling: No player obj for playerid=" << playerid << " disconnecting peer!" << std::endl;
			DeletePlayer(playerid);
		}
		return;
	}

	std::string client_formspec_name;
	StringMap fields;

	*pkt >> client_formspec_name;

	if (!pkt_read_formspec_fields(pkt, fields)) {
		warningstream << "Too large formspec fields! Ignoring for formname=\""
			<< client_formspec_name << "\", player=" << player->getName() << std::endl;
		return;
	}

	if (client_formspec_name.empty()) { // pass through inventory submits
		(reinterpret_cast<void(*)(PlayerSAO*, std::string, StringMap)>(AddonsCallbacks[CALLBACK_ON_PLAYER_RECEIVEFIELDS]))(playersao, client_formspec_name, fields);
		//m_script->on_playerReceiveFields(playersao, client_formspec_name, fields);
		return;
	}

	// verify that we displayed the formspec to the user
	const auto peer_state_iterator = m_formspec_state_data.find(peer_id);
	if (peer_state_iterator != m_formspec_state_data.end()) {
		const std::string &server_formspec_name = peer_state_iterator->second;
		if (client_formspec_name == server_formspec_name) {
			auto it = fields.find("quit");
			if (it != fields.end() && it->second == "true")
				m_formspec_state_data.erase(peer_state_iterator);

			(reinterpret_cast<void(*)(PlayerSAO*, std::string, StringMap)>(AddonsCallbacks[CALLBACK_ON_PLAYER_RECEIVEFIELDS]))(playersao, client_formspec_name, fields);
			//m_script->on_playerReceiveFields(playersao, client_formspec_name, fields);
			return;
		}
		actionstream << "'" << player->getName()
			<< "' submitted formspec ('" << client_formspec_name
			<< "') but the name of the formspec doesn't match the"
			" expected name ('" << server_formspec_name << "')";

	} else {
		actionstream << "'" << player->getName()
			<< "' submitted formspec ('" << client_formspec_name
			<< "') but server hasn't sent formspec to client";
	}
	actionstream << ", possible exploitation attempt" << std::endl;
}

//Handled by main server [If this is an slave]
void Server::handleCommand_FirstSrp(NetworkPacket* pkt)
{
	session_t peer_id = pkt->getPeerId();
	RemoteClient *client = getClient(peer_id, CS_Invalid);
	ClientState cstate = client->getState();
	const std::string playername = client->getName();

	std::string salt, verification_key;

	std::string addr_s = getPeerAddress(peer_id).serializeString();
	u8 is_empty;

	*pkt >> salt >> verification_key >> is_empty;

	verbosestream << "Server: Got TOSERVER_FIRST_SRP from " << addr_s
		<< ", with is_empty=" << (is_empty == 1) << std::endl;

	const bool empty_disallowed = !isSingleplayer() && is_empty == 1 &&
		g_settings->getBool("disallow_empty_password");

	// Either this packet is sent because the user is new or to change the password
	if (cstate == CS_HelloSent) {
		if (!client->isMechAllowed(AUTH_MECHANISM_FIRST_SRP)) {
			actionstream << "Server: Client from " << addr_s
					<< " tried to set password without being "
					<< "authenticated, or the username being new." << std::endl;
			DenyAccess(peer_id, SERVER_ACCESSDENIED_UNEXPECTED_DATA);
			return;
		}

		if (empty_disallowed) {
			actionstream << "Server: " << playername
					<< " supplied empty password from " << addr_s << std::endl;
			DenyAccess(peer_id, SERVER_ACCESSDENIED_EMPTY_PASSWORD);
			return;
		}

		std::string initial_ver_key;
		initial_ver_key = encode_srp_verifier(verification_key, salt);

		// It is possible for multiple connections to get this far with the same
		// player name. In the end only one player with a given name will be emerged
		// (see Server::StateTwoClientInit) but we still have to be careful here.
		/*if (m_script->getAuth(playername, nullptr, nullptr)) {
			// Another client beat us to it
			actionstream << "Server: Client from " << addr_s
				<< " tried to register " << playername << " a second time."
				<< std::endl;
			DenyAccess(peer_id, SERVER_ACCESSDENIED_ALREADY_CONNECTED);
			return;
		}*/
		//m_script->createAuth(playername, initial_ver_key);
		//m_script->on_authplayer(playername, addr_s, true);
		//abort();
		acceptAuth(peer_id, false);
		(reinterpret_cast<void(*)(std::string, std::string, bool)>(AddonsCallbacks[CALLBACK_ON_AUTHPLAYER]))(playername, addr_s, true);
		//abort();
		
	} else {
		//abort();
		if (cstate < CS_SudoMode) {
			infostream << "Server::ProcessData(): Ignoring TOSERVER_FIRST_SRP from "
					<< addr_s << ": " << "Client has wrong state " << cstate << "."
					<< std::endl;
			return;
		}
		m_clients.event(peer_id, CSE_SudoLeave);

		if (empty_disallowed) {
			actionstream << "Server: " << playername
					<< " supplied empty password" << std::endl;
			SendChatMessage(peer_id, ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"Changing to an empty password is not allowed."));
			return;
		}

		std::string pw_db_field = encode_srp_verifier(verification_key, salt);
		bool success = (reinterpret_cast<bool(*)(std::string, std::string)>(AddonsCallbacks[CALLBACK_SET_PASSWORD]))(playername, pw_db_field);
		//m_script->setPassword(playername, pw_db_field);
		if (success) {
			actionstream << playername << " changes password" << std::endl;
			SendChatMessage(peer_id, ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"Password change successful."));
		} else {
			actionstream << playername <<
				" tries to change password but it fails" << std::endl;
			SendChatMessage(peer_id, ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"Password change failed or unavailable."));
		}
	}
}

void Server::handleCommand_SrpBytesA(NetworkPacket* pkt)
{
	session_t peer_id = pkt->getPeerId();
	RemoteClient *client = getClient(peer_id, CS_Invalid);
	ClientState cstate = client->getState();

	if (!((cstate == CS_HelloSent) || (cstate == CS_Active))) {
		actionstream << "Server: got SRP _A packet in wrong state " << cstate <<
			" from " << getPeerAddress(peer_id).serializeString() <<
			". Ignoring." << std::endl;
		return;
	}

	const bool wantSudo = (cstate == CS_Active);

	if (client->chosen_mech != AUTH_MECHANISM_NONE) {
		actionstream << "Server: got SRP _A packet, while auth is already "
			"going on with mech " << client->chosen_mech << " from " <<
			getPeerAddress(peer_id).serializeString() <<
			" (wantSudo=" << wantSudo << "). Ignoring." << std::endl;
		if (wantSudo) {
			DenySudoAccess(peer_id);
			return;
		}

		DenyAccess(peer_id, SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	std::string bytes_A;
	u8 based_on;
	*pkt >> bytes_A >> based_on;

	infostream << "Server: TOSERVER_SRP_BYTES_A received with "
		<< "based_on=" << int(based_on) << " and len_A="
		<< bytes_A.length() << "." << std::endl;

	AuthMechanism chosen = (based_on == 0) ?
		AUTH_MECHANISM_LEGACY_PASSWORD : AUTH_MECHANISM_SRP;

	if (wantSudo) {
		if (!client->isSudoMechAllowed(chosen)) {
			actionstream << "Server: Player \"" << client->getName() <<
				"\" at " << getPeerAddress(peer_id).serializeString() <<
				" tried to change password using unallowed mech " << chosen <<
				"." << std::endl;
			DenySudoAccess(peer_id);
			return;
		}
	} else {
		if (!client->isMechAllowed(chosen)) {
			actionstream << "Server: Client tried to authenticate from " <<
				getPeerAddress(peer_id).serializeString() <<
				" using unallowed mech " << chosen << "." << std::endl;
			DenyAccess(peer_id, SERVER_ACCESSDENIED_UNEXPECTED_DATA);
			return;
		}
	}

	client->chosen_mech = chosen;

	std::string salt, verifier;

	if (based_on == 0) {

		generate_srp_verifier_and_salt(client->getName(), client->enc_pwd,
			&verifier, &salt);
	} else if (!decode_srp_verifier_and_salt(client->enc_pwd, &verifier, &salt)) {
		// Non-base64 errors should have been catched in the init handler
		actionstream << "Server: User " << client->getName() <<
			" tried to log in, but srp verifier field was invalid (most likely "
			"invalid base64)." << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_SERVER_FAIL);
		return;
	}

	char *bytes_B = 0;
	size_t len_B = 0;

	client->auth_data = srp_verifier_new(SRP_SHA256, SRP_NG_2048,
		client->getName().c_str(),
		(const unsigned char *) salt.c_str(), salt.size(),
		(const unsigned char *) verifier.c_str(), verifier.size(),
		(const unsigned char *) bytes_A.c_str(), bytes_A.size(),
		NULL, 0,
		(unsigned char **) &bytes_B, &len_B, NULL, NULL);

	if (!bytes_B) {
		actionstream << "Server: User " << client->getName()
			<< " tried to log in, SRP-6a safety check violated in _A handler."
			<< std::endl;
		if (wantSudo) {
			DenySudoAccess(peer_id);
			client->resetChosenMech();
			return;
		}

		DenyAccess(peer_id, SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	NetworkPacket resp_pkt(TOCLIENT_SRP_BYTES_S_B, 0, peer_id);
	resp_pkt << salt << std::string(bytes_B, len_B);
	Send(&resp_pkt);
}

void Server::handleCommand_SrpBytesM(NetworkPacket* pkt)
{
	session_t peer_id = pkt->getPeerId();
	RemoteClient *client = getClient(peer_id, CS_Invalid);
	ClientState cstate = client->getState();
	const std::string addr_s = client->getAddress().serializeString();
	const std::string playername = client->getName();

	const bool wantSudo = (cstate == CS_Active);

	verbosestream << "Server: Received TOSERVER_SRP_BYTES_M." << std::endl;

	if (!((cstate == CS_HelloSent) || (cstate == CS_Active))) {
		warningstream << "Server: got SRP_M packet in wrong state "
			<< cstate << " from " << addr_s << ". Ignoring." << std::endl;
		return;
	}

	if (client->chosen_mech != AUTH_MECHANISM_SRP &&
			client->chosen_mech != AUTH_MECHANISM_LEGACY_PASSWORD) {
		warningstream << "Server: got SRP_M packet, while auth "
			"is going on with mech " << client->chosen_mech << " from "
			<< addr_s << " (wantSudo=" << wantSudo << "). Denying." << std::endl;
		if (wantSudo) {
			DenySudoAccess(peer_id);
			return;
		}

		DenyAccess(peer_id, SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	std::string bytes_M;
	*pkt >> bytes_M;

	if (srp_verifier_get_session_key_length((SRPVerifier *) client->auth_data)
			!= bytes_M.size()) {
		actionstream << "Server: User " << playername << " at " << addr_s
			<< " sent bytes_M with invalid length " << bytes_M.size() << std::endl;
		DenyAccess(peer_id, SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	unsigned char *bytes_HAMK = 0;

	srp_verifier_verify_session((SRPVerifier *) client->auth_data,
		(unsigned char *)bytes_M.c_str(), &bytes_HAMK);

	if (!bytes_HAMK) {
		if (wantSudo) {
			actionstream << "Server: User " << playername << " at " << addr_s
				<< " tried to change their password, but supplied wrong"
				<< " (SRP) password for authentication." << std::endl;
			DenySudoAccess(peer_id);
			client->resetChosenMech();
			return;
		}

		actionstream << "Server: User " << playername << " at " << addr_s
			<< " supplied wrong password (auth mechanism: SRP)." << std::endl;
		//m_script->on_authplayer(playername, addr_s, false);
		(reinterpret_cast<void(*)(std::string, std::string, bool)>(AddonsCallbacks[CALLBACK_ON_AUTHPLAYER]))(playername, addr_s, false);
		DenyAccess(peer_id, SERVER_ACCESSDENIED_WRONG_PASSWORD);
		return;
	}

	/*if (client->create_player_on_auth_success) {
		m_script->createAuth(playername, client->enc_pwd);

		if (!m_script->getAuth(playername, nullptr, nullptr)) {
			errorstream << "Server: " << playername <<
				" cannot be authenticated (auth handler does not work?)" <<
				std::endl;
			DenyAccess(peer_id, SERVER_ACCESSDENIED_SERVER_FAIL);
			return;
		}
		client->create_player_on_auth_success = false;
	}*/

	//m_script->on_authplayer(playername, addr_s, true);
	(reinterpret_cast<void(*)(std::string, std::string, bool)>(AddonsCallbacks[CALLBACK_ON_AUTHPLAYER]))(playername, addr_s, true);
	acceptAuth(peer_id, wantSudo);
}

