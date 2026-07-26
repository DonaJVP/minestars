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

//The purpose of this file is to replace the clientiface functions when this server acts like a slave

#pragma once

#include "irr_v3d.h"                   // for irrlicht datatypes

#include "constants.h"
#include "serialization.h"             // for SER_FMT_VER_INVALID
#include "network/networkpacket.h"
#include "network/networkprotocol.h"
#include "network/address.h"
#include "porting.h"
#include "clientiface.h"
#include "settings.h"
#include "slave_proxy_net/objects_id_logic.h"

#include <list>
#include <vector>
#include <set>
#include <memory>
#include <mutex>

class Server;


struct PrioritySortedBlockTransferPID
{
	PrioritySortedBlockTransferPID(float a_priority, const v3s16 &a_pos, u16 pid)
	{
		priority = a_priority;
		pos = a_pos;
		playerid = pid;
	}
	bool operator < (const PrioritySortedBlockTransferPID &other) const
	{
		return priority < other.priority;
	}
	float priority;
	v3s16 pos;
	u16 playerid;
};

/*
	ret.state = client->getState();
	ret.addr = client->getAddress();
	ret.uptime = client->uptime();
	ret.ser_vers = client->serialization_version;
	ret.prot_vers = client->net_proto_version;

	ret.major = client->getMajor();
	ret.minor = client->getMinor();
	ret.patch = client->getPatch();
	ret.vers_string = client->getFullVer();
	ret.platform = client->getPlatform();
	ret.sysinfo = client->getSysInfo();

	ret.lang_code = client->getLangCode();
*/

/*
	m_max_simul_sends(g_settings->getU16("max_simultaneous_block_sends_per_client")),
	m_min_time_from_building(g_settings->getFloat("full_block_send_enable_min_time_from_building")),
	m_max_send_distance(g_settings->getS16("max_block_send_distance")),
	m_block_optimize_distance(g_settings->getS16("block_send_optimize_distance")),
	m_max_gen_distance(g_settings->getS16("max_block_generate_distance")),
	m_occ_cull(g_settings->getBool("server_side_occlusion_culling"))
*/

class ClientDataHelper {
	public:
		ClientDataHelper() : m_max_simul_sends(g_settings->getU16("max_simultaneous_block_sends_per_client")),
		m_min_time_from_building(g_settings->getFloat("full_block_send_enable_min_time_from_building")),
		m_max_send_distance(g_settings->getS16("max_block_send_distance")),
		m_block_optimize_distance(g_settings->getS16("block_send_optimize_distance")),
		m_max_gen_distance(g_settings->getS16("max_block_generate_distance")),
		m_occ_cull(g_settings->getBool("server_side_occlusion_culling")) {};
		
		~ClientDataHelper() = default;
		void step(float dtime);
		void GetNextBlocks(ServerEnvironment *env, EmergeManager* emerge, float dtime, std::vector<PrioritySortedBlockTransferPID> &dest);
		void SentBlock(v3s16 p);
		u16 GetSaoID();
		u32 getSendingCount() const { return m_blocks_sending.size(); }
		bool isBlockSent(v3s16 p) const { return m_blocks_sent.find(p) != m_blocks_sent.end(); }
		void SetBlockNotSent(v3s16 p);
		void SetBlocksNotSent(std::map<v3s16, MapBlock*> &blocks);
		std::set<u16> m_known_objects;
		//data about player
		u16 GetPlayerID_() { return playerid; }
		std::string getName() const { return m_name; }
		void setName(const std::string &name) { m_name = name; }
		ClientState getState() const { return CS_InitDone; } //??
		const Address &getAddress() const { return m_addr; }
		u64 uptime() const;
		u8 getMajor() const { return m_version_major; }
		u8 getMinor() const { return m_version_minor; }
		u8 getPatch() const { return m_version_patch; }
		const std::string &getFullVer() const { return m_full_version; }
		const std::string &getPlatform() const { return m_platform; }
		const std::string &getSysInfo() const { return m_sysinfo; }
		void setServerObj(Server *env) { server = env; }
		void setSerializationVer(u8 val) { serialization_version = val; }
		void setTrueSAOId(u16 ID) { IdInSlave = ID; }
		
		//save/store funcs
		void setMajor(u8 v) { m_version_major = v; }
		void setMinor(u8 v) { m_version_minor = v; }
		void setPatch(u8 v) { m_version_patch = v; }
		void setFullVer(std::string v) { m_full_version = v; }
		void setPlatform(std::string v) { m_platform = v; }
		void setSysInfo(std::string v) { errorstream << "At setSysInfo(std::string v): Nope" << std::endl; }
		void setPID(u16 PID) { playerid = PID; }
		void setProto(u16 p) { net_proto_version = p; }
		const std::string &getLangCode() const { return m_lang_code; }
		u16 net_proto_version = 0;
		u8 serialization_version = SER_FMT_VER_INVALID;
		void GotBlock(v3s16 p);
		bool FirstStep = true;
		
		u16 IdInSlave = 0; //sao ID
		float countToInitialize = 1.0f; //Initialize player after the removal of all known entities by the player
		
		MasterServerUniqueIDS *SuperIDs;
		
		void ResendBlockIfOnWire(v3s16 p);
		
		u16 known_players_count = 0;
		
		float m_time_from_building = 9999;
	private:
		u32 m_excess_gotblocks = 0;
		u16 m_max_simul_sends;
		float m_min_time_from_building;
		s16 m_max_send_distance;
		s16 m_block_optimize_distance;
		s16 m_max_gen_distance;
		bool m_occ_cull;
		u16 playerid = 0;
		const u64 m_connection_time = porting::getTimeS();
		float m_nothing_to_send_pause_timer = 0.0f;
		std::map<v3s16, float> m_blocks_sending;
		v3s16 m_last_center;
		s16 m_nearest_unsent_d = 0;
		v3f m_last_camera_dir;
		std::set<v3s16> m_blocks_modified;
		std::set<v3s16> m_blocks_sent;
		std::string m_name = "";
		u8 m_version_major = 0;
		u8 m_version_minor = 0;
		u8 m_version_patch = 0;
		std::string m_full_version = "unknown";
		std::string m_platform = "unknown"; //x
		std::string m_sysinfo = "nope";
		std::string m_lang_code = "nope";
		//May connection should return nowhere?
		Address m_addr;
		//config
		Server *server = nullptr;
};









