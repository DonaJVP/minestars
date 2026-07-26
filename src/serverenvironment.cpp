/*
Minetest
Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <algorithm>
#include <cstdint>
#include "serverenvironment.h"
#include "clientiface.h"
#include "debug.h"
#include "settings.h"
#include "log.h"
#include "mapblock.h"
#include "nodedef.h"
#include "nodemetadata.h"
#include "gamedef.h"
#include "map.h"
#include "porting.h"
#include "profiler.h"
#include "raycast.h"
#include "remoteplayer.h"
#include "abm_threading.h"
#include "scripting_server.h"
#include "server.h"
#include "tool.h"
#include "util/serialize.h"
#include "util/basic_macros.h"
#include "util/pointedthing.h"
#include "threading/mutex_auto_lock.h"
#include "filesys.h"
#include "gameparams.h"
#include "database/database-dummy.h"
#include "database/database-files.h"
#if USE_SQLITE
#include "database/database-sqlite3.h"
#endif
#if USE_POSTGRESQL
#include "database/database-postgresql.h"
#endif
#if USE_LEVELDB
#include "database/database-leveldb.h"
#endif
#include "server/luaentity_sao.h"
#include "server/player_sao.h"
#include "ServerNetworkEngine.h"
//#include "server/unit_sao.h"
#include "network/networkpacket.h"
#include "main.h"
#include "addons/cblks_def.hpp"
#include "auth.h"

#define LBM_NAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyz0123456789_:"

// A number that is much smaller than the timeout for particle spawners should/could ever be
#define PARTICLE_SPAWNER_NO_EXPIRY -1024.f

/*
	ABMWithState
*/



/*
	LBMManager
*/

void LBMContentMapping::deleteContents()
{
	for (auto &it : lbm_list) {
		delete it;
	}
}

void LBMContentMapping::addLBM(LoadingBlockModifierDef *lbm_def, IGameDef *gamedef)
{
	// Add the lbm_def to the LBMContentMapping.
	// Unknown names get added to the global NameIdMapping.
	const NodeDefManager *nodedef = gamedef->ndef();

	lbm_list.push_back(lbm_def);

	for (const std::string &nodeTrigger: lbm_def->trigger_contents) {
		std::vector<content_t> c_ids;
		bool found = nodedef->getIds(nodeTrigger, c_ids);
		if (!found) {
			content_t c_id = gamedef->allocateUnknownNodeId(nodeTrigger);
			if (c_id == CONTENT_IGNORE) {
				// Seems it can't be allocated.
				warningstream << "Could not internalize node name \"" << nodeTrigger
					<< "\" while loading LBM \"" << lbm_def->name << "\"." << std::endl;
				continue;
			}
			c_ids.push_back(c_id);
		}

		for (content_t c_id : c_ids) {
			map[c_id].push_back(lbm_def);
		}
	}
}

const std::vector<LoadingBlockModifierDef *> *
LBMContentMapping::lookup(content_t c) const
{
	lbm_map::const_iterator it = map.find(c);
	if (it == map.end())
		return NULL;
	// This first dereferences the iterator, returning
	// a std::vector<LoadingBlockModifierDef *>
	// reference, then we convert it to a pointer.
	return &(it->second);
}

LBMManager::~LBMManager()
{
	for (auto &m_lbm_def : m_lbm_defs) {
		delete m_lbm_def.second;
	}

	for (auto &it : m_lbm_lookup) {
		(it.second).deleteContents();
	}
}

void LBMManager::addLBMDef(LoadingBlockModifierDef *lbm_def)
{
	// Precondition, in query mode the map isn't used anymore
	FATAL_ERROR_IF(m_query_mode,
		"attempted to modify LBMManager in query mode");

	if (!string_allowed(lbm_def->name, LBM_NAME_ALLOWED_CHARS)) {
		throw ModError("Error adding LBM \"" + lbm_def->name +
			"\": Does not follow naming conventions: "
				"Only characters [a-z0-9_:] are allowed.");
	}

	m_lbm_defs[lbm_def->name] = lbm_def;
}

void LBMManager::loadIntroductionTimes(const std::string &times,
	IGameDef *gamedef, u32 now)
{
	m_query_mode = true;

	// name -> time map.
	// Storing it in a map first instead of
	// handling the stuff directly in the loop
	// removes all duplicate entries.
	// TODO make this std::unordered_map
	std::map<std::string, u32> introduction_times;

	/*
	The introduction times string consists of name~time entries,
	with each entry terminated by a semicolon. The time is decimal.
	 */

	size_t idx = 0;
	size_t idx_new;
	while ((idx_new = times.find(';', idx)) != std::string::npos) {
		std::string entry = times.substr(idx, idx_new - idx);
		std::vector<std::string> components = str_split(entry, '~');
		if (components.size() != 2)
			throw SerializationError("Introduction times entry \""
				+ entry + "\" requires exactly one '~'!");
		const std::string &name = components[0];
		u32 time = from_string<u32>(components[1]);
		introduction_times[name] = time;
		idx = idx_new + 1;
	}

	// Put stuff from introduction_times into m_lbm_lookup
	for (std::map<std::string, u32>::const_iterator it = introduction_times.begin();
		it != introduction_times.end(); ++it) {
		const std::string &name = it->first;
		u32 time = it->second;

		std::map<std::string, LoadingBlockModifierDef *>::iterator def_it =
			m_lbm_defs.find(name);
		if (def_it == m_lbm_defs.end()) {
			// This seems to be an LBM entry for
			// an LBM we haven't loaded. Discard it.
			continue;
		}
		LoadingBlockModifierDef *lbm_def = def_it->second;
		if (lbm_def->run_at_every_load) {
			// This seems to be an LBM entry for
			// an LBM that runs at every load.
			// Don't add it just yet.
			continue;
		}

		m_lbm_lookup[time].addLBM(lbm_def, gamedef);

		// Erase the entry so that we know later
		// what elements didn't get put into m_lbm_lookup
		m_lbm_defs.erase(name);
	}

	// Now also add the elements from m_lbm_defs to m_lbm_lookup
	// that weren't added in the previous step.
	// They are introduced first time to this world,
	// or are run at every load (introducement time hardcoded to U32_MAX).

	LBMContentMapping &lbms_we_introduce_now = m_lbm_lookup[now];
	LBMContentMapping &lbms_running_always = m_lbm_lookup[U32_MAX];

	for (auto &m_lbm_def : m_lbm_defs) {
		if (m_lbm_def.second->run_at_every_load) {
			lbms_running_always.addLBM(m_lbm_def.second, gamedef);
		} else {
			lbms_we_introduce_now.addLBM(m_lbm_def.second, gamedef);
		}
	}

	// Clear the list, so that we don't delete remaining elements
	// twice in the destructor
	m_lbm_defs.clear();
}

std::string LBMManager::createIntroductionTimesString()
{
	// Precondition, we must be in query mode
	FATAL_ERROR_IF(!m_query_mode,
		"attempted to query on non fully set up LBMManager");

	std::ostringstream oss;
	for (const auto &it : m_lbm_lookup) {
		u32 time = it.first;
		const std::vector<LoadingBlockModifierDef *> &lbm_list = it.second.lbm_list;
		for (const auto &lbm_def : lbm_list) {
			// Don't add if the LBM runs at every load,
			// then introducement time is hardcoded
			// and doesn't need to be stored
			if (lbm_def->run_at_every_load)
				continue;
			oss << lbm_def->name << "~" << time << ";";
		}
	}
	return oss.str();
}

void LBMManager::applyLBMs(ServerEnvironment *env, MapBlock *block, u32 stamp)
{
	// Precondition, we need m_lbm_lookup to be initialized
	FATAL_ERROR_IF(!m_query_mode,
		"attempted to query on non fully set up LBMManager");
	v3s16 pos_of_block = block->getPosRelative();
	v3s16 pos;
	MapNode n;
	content_t c;
	lbm_lookup_map::const_iterator it = getLBMsIntroducedAfter(stamp);
	for (; it != m_lbm_lookup.end(); ++it) {
		// Cache previous version to speedup lookup which has a very high performance
		// penalty on each call
		content_t previous_c{};
		std::vector<LoadingBlockModifierDef *> *lbm_list = nullptr;

		for (pos.X = 0; pos.X < MAP_BLOCKSIZE; pos.X++)
			for (pos.Y = 0; pos.Y < MAP_BLOCKSIZE; pos.Y++)
				for (pos.Z = 0; pos.Z < MAP_BLOCKSIZE; pos.Z++) {
					n = block->getNodeNoEx(pos);
					c = n.getContent();

					// If content_t are not matching perform an LBM lookup
					if (previous_c != c) {
						lbm_list = (std::vector<LoadingBlockModifierDef *> *)
							it->second.lookup(c);
						previous_c = c;
					}

					if (!lbm_list)
						continue;
					for (auto lbmdef : *lbm_list) {
						lbmdef->trigger(env, pos + pos_of_block, n);
					}
				}
	}
}



/*
	ServerEnvironment
*/

// Random device to seed pseudo random generators.
static std::random_device seed;

ServerEnvironment::ServerEnvironment(ServerMap *map, ServerScripting *scriptIface, Server *server, const std::string &path_world,
std::unordered_map<uint16_t, HyperMap*> *maps):
	Environment(server),
	m_map(map),
	m_script(scriptIface),
	m_server(server),
	m_path_world(path_world),
	m_rgen(seed()),
	Maps(maps)
{
	// Determine which database backend to use
	std::string conf_path = path_world + DIR_DELIM + "serverprop.mt";
	Settings conf;

#if !defined(__ANDROID__) && !defined(__APPLE__)
	std::string player_backend_name = "sqlite3";
	std::string auth_backend_name = "sqlite3";
#else
	std::string player_backend_name = "leveldb";
	std::string auth_backend_name = "leveldb";
#endif

	bool succeeded = conf.readConfigFile(conf_path.c_str());

	// If we open world.mt read the backend configurations.
	if (succeeded) {
		// Read those values before setting defaults
		bool player_backend_exists = conf.exists("player_backend");
		bool auth_backend_exists = conf.exists("auth_backend");

		// player backend is not set, assume it's legacy file backend.
		if (!player_backend_exists) {
			// fall back to files
			conf.set("player_backend", "files");
			player_backend_name = "files";

			if (!conf.updateConfigFile(conf_path.c_str())) {
				errorstream << "ServerEnvironment::ServerEnvironment(): "
						<< "Failed to update serverprop.mt!" << std::endl;
			}
		} else {
			conf.getNoEx("player_backend", player_backend_name);
		}

		// auth backend is not set, assume it's legacy file backend.
		if (!auth_backend_exists) {
			conf.set("auth_backend", "files");
			auth_backend_name = "files";

			if (!conf.updateConfigFile(conf_path.c_str())) {
				errorstream << "ServerEnvironment::ServerEnvironment(): "
						<< "Failed to update serverprop.mt!" << std::endl;
			}
		} else {
			conf.getNoEx("auth_backend", auth_backend_name);
		}
	}


	if (player_backend_name == "files") {
		warningstream << "/!\\ You are using old player file backend. "
				<< "This backend is deprecated and will be removed in a future release /!\\"
				<< std::endl << "Switching to SQLite3 or PostgreSQL is advised, "
				<< "please read http://wiki.minetest.net/Database_backends." << std::endl;
	}

	if (auth_backend_name == "files") {
		warningstream << "/!\\ You are using old auth file backend. "
				<< "This backend is deprecated and will be removed in a future release /!\\"
				<< std::endl << "Switching to LevelDB or SQLite3 is advised, "
				<< "please read http://wiki.minetest.net/Database_backends." << std::endl;
	}


	m_player_database = openPlayerDatabase(player_backend_name, path_world, conf);
	m_auth_database = openAuthDatabase(auth_backend_name, path_world, conf);
	_AUTH__setDatabaseObject(m_auth_database);

	m_compat_send_original_model = !server->getCompatPlayerModels().empty() &&
			g_settings->getBool("compat_send_original_model");
	
	m_cache_abm_interval = g_settings->getFloat("abm_interval");
}

ServerEnvironment::~ServerEnvironment()
{
	// Clear active block list.
	// This makes the next one delete all active objects.
	//m_active_blocks.clear();

	// Convert all objects to static and delete the active objects
	deactivateFarObjects(true);

	// Drop/delete map
	//m_map->drop();

	for (auto i = Maps->begin(); i != Maps->end(); i++) {
		actionstream << FUNCTION_NAME << ": Dropping map: " << i->second->m_name << std::endl;
		i->second->m_map->drop();
	}

	// Delete ActiveBlockModifiers
	//for (ABMWithState &m_abm : m_abms) {
	//	delete m_abm.abm;
	//}

	// Deallocate players
	for (RemotePlayer *m_player : m_players) {
		delete m_player;
	}

	delete m_player_database;
	delete m_auth_database;
}

//Returns the main map
Map & ServerEnvironment::getMap(uint16_t mapid)
{
	return *(Maps->at(mapid)->m_map);
}
ServerMap & ServerEnvironment::getServerMap(uint16_t mapid)
{
	return *(Maps->at(mapid)->m_map); //If not specified it will always return the raw map
}

RemotePlayer *ServerEnvironment::getPlayerByID(const uint16_t id) {
	if (StoredPlayersIDs.find(id) != StoredPlayersIDs.end()) {
		return StoredPlayersIDs.at(id);
	} else {
		return nullptr;
	}
}

// Well, PlayerID and PeerID now is one, (The playerID starts by 4500, i don't think the server would get more users than 300 . .
// servers.minetest.net shows not more than 451 users, divided by those 300 servers)
RemotePlayer *ServerEnvironment::getPlayer(const session_t peer_id)
{
	/*for (RemotePlayer *player : m_players) {
		if (player->getPeerId() == peer_id)
			return player;
	}*/
	// Better use vector for less cpu usage (What i'm doing if SNE and MAPs uses the 70%)
	RemotePlayer *ply = NULL;
	try {
		ply = m_players.at(peer_id);
	} catch (const std::out_of_range& huh) {
		//warningstream << FUNCTION_NAME << ": Player not found: " << peer_id << std::endl;
		return NULL;
	}
	return ply;
}

RemotePlayer *ServerEnvironment::getPlayer(const char* name)
{
	//NOTE: Disabled due to excess of jobs for cpu
	/*for (RemotePlayer *player : m_players) {
		if (strcmp(player->getName(), name) == 0)
			return player;
	}*/
	if (PnameToID.find(std::string(name)) != PnameToID.end()) {
		return m_players.at(PnameToID.at(std::string(name)));
	}
	return NULL;
}

void ServerEnvironment::addPlayer(RemotePlayer *player)
{
	// If peer id is non-zero, it has to be unique.
	if (!m_server->ServersNetworkObject->AreSlave) {
		if (player->getPeerId() == PEER_ID_INEXISTENT)
			FATAL_ERROR_IF(getPlayer(player->getPeerId()) != NULL, "Peer id not unique");
		// Name has to be unique.
		//FATAL_ERROR_IF(getPlayer(player->getName()) != NULL, "Player name not unique");
	}
	// Add.
	actionstream << "Adding player: " << player->player_id << std::endl;
	StoredPlayersIDs[player->player_id] = player;
	
	if (m_players.size() < player->player_id)
		m_players.resize(player->player_id+1);
	m_players[player->player_id] = player; //PlayerID
	m_players[player->getPeerId()] = player; //PeerID
	
}

void ServerEnvironment::removePlayer(RemotePlayer *player)
{
	StoredPlayersIDs.erase(player->player_id); //delete an stored id
	for (std::vector<RemotePlayer *>::iterator it = m_players.begin();
		it != m_players.end(); ++it) {
		if ((*it) == player) {
			u16 id = DirectionsMAP[player->getPeerId()];
			if (id != 0) {
				PlayerMAPwithOBJID.erase(id);
				DirectionsMAP.erase(player->getPeerId());
			}
			delete *it;
			m_players.erase(it);
			return;
		}
	}
}

bool ServerEnvironment::removePlayerFromDatabase(const std::string &name)
{
	return m_player_database->removePlayer(name);
}

void ServerEnvironment::kickAllPlayers(AccessDeniedCode reason,
	const std::string &str_reason, bool reconnect)
{
	for (RemotePlayer *player : m_players)
		m_server->DenyAccess(player->getPeerId(), reason, str_reason, reconnect);
}

void ServerEnvironment::saveLoadedPlayers(bool force)
{
	for (auto i = PnameToID.begin(); i != PnameToID.end(); i++) {
		RemotePlayer *player = getPlayer(i->second);
		if (!player)
			continue;
		//If the player are playing in other environment, discard him
		if (player->to_other_server)
			continue;
		if (force || player->checkModified() || (player->getPlayerSAO() &&
				player->getPlayerSAO()->getMeta().isModified())) {
			try {
				m_player_database->savePlayer(player);
			} catch (DatabaseException &e) {
				errorstream << "Failed to save player " << player->getName() << " exception: "
					<< e.what() << std::endl;
				throw;
			}
		}
	}
}

void ServerEnvironment::savePlayer(RemotePlayer *player)
{
	try {
		m_player_database->savePlayer(player);
	} catch (DatabaseException &e) {
		errorstream << "Failed to save player " << player->getName() << " exception: "
			<< e.what() << std::endl;
		throw;
	}
}

PlayerSAO *ServerEnvironment::loadPlayer(RemotePlayer *player, bool *new_player, uint16_t peer_id, uint16_t mapid)
{
	PlayerSAO *playersao = new PlayerSAO(this, player, peer_id, false);
	player->setPeerId(peer_id);
	// Create player if it doesn't exist
	if (!m_player_database->loadPlayer(player, playersao)) {
		*new_player = true;
		// Set player position
		infostream << "Server: Finding spawn place for player \"" << player->getName() << "\"" << std::endl;
		playersao->setBasePosition(m_server->findSpawnPos(mapid));
		// Make sure the player is saved
		player->setModified(true);
	} else {
		// If the player exists, ensure that they respawn inside legal bounds
		// This fixes an assert crash when the player can't be added
		// to the environment
		if (objectpos_over_limit(playersao->getBasePosition())) {
			actionstream << "Respawn position for player \"" << player->getName() << "\" outside limits, resetting" << std::endl;
			playersao->setBasePosition(m_server->findSpawnPos(mapid));
		}
	}

	// Add player to environment
	addPlayer(player);

	/* Clean up old HUD elements from previous sessions */
	player->clearHud();

	PnameToID[player->getName()] = peer_id;

	/* Add object to environment */
	if (!SLAVE_INSTANCE.load()) {
		u16 id = addActiveObject(playersao);
		PlayerMAPwithOBJID[id] = 0; //Proxy has nothing to do here
		DirectionsMAP[peer_id] = id;
	} else {
		u16 ID_FOR_PLAYER = addActiveObject(playersao);
		player->IdForSao = ID_FOR_PLAYER;
		PlayerMAPwithOBJID[ID_FOR_PLAYER] = peer_id;
		DirectionsMAP[peer_id] = ID_FOR_PLAYER;
	}
	return playersao;
}

RemotePlayer *ServerEnvironment::FindPlayerWithThisId(u16 id) {
	if (PlayerMAPwithOBJID.find(id) == PlayerMAPwithOBJID.end())
		return NULL;
	return getPlayer(PlayerMAPwithOBJID.at(id));
}

//Player index
const std::vector<RemotePlayer *> ServerEnvironment::getPlayers() {
	std::vector<RemotePlayer*> list;
	for (auto i = PnameToID.begin(); i != PnameToID.end(); i++) {
		RemotePlayer *p = getPlayer(i->second);
		if (p && !p->OnServer->IsApplied)
			list.push_back(p);
	}
	return list;
}

u32 ServerEnvironment::getPlayerCount() {
	u32 count = 0;
	for (auto i = PnameToID.begin(); i != PnameToID.end(); i++) {
		if (getPlayer(i->second) && !getPlayer(i->second)->OnServer->IsApplied)
			count++;
	}
	return count;
}

void ServerEnvironment::saveMeta() {
	if (!m_meta_loaded)
		return;

	std::string path = m_path_world + DIR_DELIM "environment.msd";

	// Open file and serialize
	std::ostringstream ss(std::ios_base::binary);

	/*Settings args("EnvArgsEnd");
	args.setU64("game_time", m_game_time);
	args.setU64("time_of_day", getTimeOfDay());
	args.setU64("last_clear_objects_time", m_last_clear_objects_time);
	args.setU64("lbm_introduction_times_version", 1);
	args.set("lbm_introduction_times", m_lbm_mgr.createIntroductionTimesString());
	args.setU64("day_count", m_day_count);
	if (m_has_world_spawnpoint)
		args.setV3F("static_spawnpoint", m_world_spawnpoint);
	args.writeLines(ss);
	*/

	//Use raw binary data for less data manipulation.
	/*
	 * [0] = Game time
	 * [4] = Time of Day
	 * [8] = Clear Objs Time
	 * [12] = Day count
	 * [16] = lbm_introduction_times_version [Why it is 1?] {Made to 16bits}
	 * [18] = Size of lbm_introduction_times in 16bits
	 * [18..X] = String of lbm_introduction_times
	 * [X] = Spawn pos
	 */
	std::string itr = m_lbm_mgr.createIntroductionTimesString();
	std::string data;
	data.append("MineStars::EnvironmentData::v1.0");
	char buff[8];
	writeU64((uint8_t*)buff, m_game_time);
	data.append(buff, 8);
	writeU64((uint8_t*)buff, getTimeOfDay());
	data.append(buff, 8);
	writeU64((uint8_t*)buff, m_last_clear_objects_time);
	data.append(buff, 8);
	writeU64((uint8_t*)buff, m_day_count);
	data.append(buff, 8);
	writeU16((uint8_t*)buff, 1);
	data.append(buff, 2);
	writeU16((uint8_t*)buff, itr.size());
	data.append(buff, 2);
	data.append(itr);

	ss << data;

	if(!fs::safeWriteToFile(path, ss.str()))
	{
		errorstream << "ServerEnvironment::saveMeta(): Failed to write " << path << "; This might result in data loss" << std::endl;
		return;
	}
}

void ServerEnvironment::loadMeta()
{
	SANITY_CHECK(!m_meta_loaded);
	m_meta_loaded = true;

	// If file doesn't exist, load default environment metadata
	if (!fs::PathExists(m_path_world + DIR_DELIM "environment.msd")) {
		infostream << "ServerEnvironment: Loading default environment metadata" << std::endl;
		loadDefaultMeta();
		return;
	}

	std::string path = m_path_world + DIR_DELIM "environment.msd";

	// Open file and deserialize
	std::ifstream is(path.c_str(), std::ios_base::binary);
	if (!is.good()) {
		warningstream << "ServerEnvironment::loadMeta(): Failed to open " << path << "; Using fallback options" << std::endl;
		loadDefaultMeta();
		return; //Don't crash if the MineStarsData are not found
	}

	infostream << FUNCTION_NAME << ": Going to load environment data <environment.msd>" << std::endl;
	//Get per blocks
	std::ostringstream os(std::ios_base::binary);
	bool bad = false;
	char buf[4096];
	for(;;){
		is.read(buf, sizeof(buf));
		std::streamsize len = is.gcount();
		os.write(buf, len);
		if(is.eof())
			break;
		if(!is.good()){
			bad = true;
			break;
		}
	}
	if (bad) {
		warningstream << FUNCTION_NAME << ": Could not load environment data . . . ." << std::endl;
		loadDefaultMeta();
	}

	//Acquire data for process
	std::string rawdata_;
	rawdata_.append(os.str());
	std::vector<uint8_t> rawdata(rawdata_.begin(), rawdata_.end());
	//Let's remembet, that theres a string which is 32bytes long
	uint32_t pos = 32;

	//Unsigned long long int
	m_game_time = ((uint64_t)rawdata[pos] << 56) | ((uint64_t)rawdata[pos+1] << 48) | ((uint64_t)rawdata[pos+2] << 40) | ((uint64_t)rawdata[pos+3] << 32) | ((uint64_t)rawdata[pos+4] << 24) | ((uint64_t)rawdata[pos+5] << 16) | ((uint64_t)rawdata[pos+6] <<  8) | ((uint64_t)rawdata[pos+7] << 0);
	pos += 8;

	setTimeOfDay(((uint64_t)rawdata[pos] << 56) | ((uint64_t)rawdata[pos+1] << 48) | ((uint64_t)rawdata[pos+2] << 40) | ((uint64_t)rawdata[pos+3] << 32) | ((uint64_t)rawdata[pos+4] << 24) | ((uint64_t)rawdata[pos+5] << 16) | ((uint64_t)rawdata[pos+6] <<  8) | ((uint64_t)rawdata[pos+7] << 0));
	pos += 8;

	m_last_clear_objects_time = ((uint64_t)rawdata[pos] << 56) | ((uint64_t)rawdata[pos+1] << 48) | ((uint64_t)rawdata[pos+2] << 40) | ((uint64_t)rawdata[pos+3] << 32) | ((uint64_t)rawdata[pos+4] << 24) | ((uint64_t)rawdata[pos+5] << 16) | ((uint64_t)rawdata[pos+6] <<  8) | ((uint64_t)rawdata[pos+7] << 0);
	pos += 8;

	m_day_count = ((uint64_t)rawdata[pos] << 56) | ((uint64_t)rawdata[pos+1] << 48) | ((uint64_t)rawdata[pos+2] << 40) | ((uint64_t)rawdata[pos+3] << 32) | ((uint64_t)rawdata[pos+4] << 24) | ((uint64_t)rawdata[pos+5] << 16) | ((uint64_t)rawdata[pos+6] <<  8) | ((uint64_t)rawdata[pos+7] << 0);
	pos += 8;

	//Unsigned short int
	std::string lbm_introduction_times;
	uint16_t ver = ((uint16_t)rawdata[pos] << 8) | ((uint16_t)rawdata[pos+1] << 0);
	pos += 2;
	if (ver != 1) {
		warningstream << FUNCTION_NAME << ": Unsupported introduction_times version: " << ver << std::endl;
		//lbm_introduction_times = "";
	} else {
		//Load string
		uint16_t size = ((uint16_t)rawdata[pos] << 8) | ((uint16_t)rawdata[pos+1] << 0);
		std::string lit(&rawdata[pos], &rawdata[pos+size]);
		lbm_introduction_times = lit;
		pos += size;
	}
	m_lbm_mgr.loadIntroductionTimes(lbm_introduction_times, m_server, m_game_time);
}

/**
 * called if env_meta.txt doesn't exist (e.g. new world)
 */
void ServerEnvironment::loadDefaultMeta()
{
	m_lbm_mgr.loadIntroductionTimes("", m_server, m_game_time);
}


void ServerEnvironment::activateBlock(MapBlock *block, u32 additional_dtime, uint16_t mapid)
{
	// Reset usage timer immediately, otherwise a block that becomes active
	// again at around the same time as it would normally be unloaded will
	// get unloaded incorrectly. (I think this still leaves a small possibility
	// of a race condition between this and server::AsyncRunStep, which only
	// some kind of synchronisation will fix, but it at least reduces the window
	// of opportunity for it to break from seconds to nanoseconds)
	block->resetUsageTimer();

	// Get time difference
	u32 dtime_s = 0;
	u32 stamp = block->getTimestamp();
	if (m_game_time > stamp && stamp != BLOCK_TIMESTAMP_UNDEFINED)
		dtime_s = m_game_time - stamp;
	dtime_s += additional_dtime;

	/*infostream<<"ServerEnvironment::activateBlock(): block timestamp: "
			<<stamp<<", game time: "<<m_game_time<<std::endl;*/

	// Remove stored static objects if clearObjects was called since block's timestamp
	if (stamp == BLOCK_TIMESTAMP_UNDEFINED || stamp < m_last_clear_objects_time) {
		block->m_static_objects.m_stored.clear();
		// do not set changed flag to avoid unnecessary mapblock writes
	}

	// Set current time as timestamp
	block->setTimestampNoChangedFlag(m_game_time);

	/*infostream<<"ServerEnvironment::activateBlock(): block is "
			<<dtime_s<<" seconds old."<<std::endl;*/

	// Activate stored objects
	activateObjects(block, dtime_s, mapid);

	/* Handle LoadingBlockModifiers */
	m_lbm_mgr.applyLBMs(this, block, stamp);

	// Run node timers
	std::vector<NodeTimer> elapsed_timers =
		block->m_node_timers.step((float)dtime_s);
	if (!elapsed_timers.empty()) {
		MapNode n;
		for (const NodeTimer &elapsed_timer : elapsed_timers) {
			n = block->getNodeNoEx(elapsed_timer.position);
			v3s16 p = elapsed_timer.position + block->getPosRelative();
			//if (m_script->node_on_timer(p, n, elapsed_timer.elapsed))
			if ((reinterpret_cast<bool(*)(v3s16, MapNode, float)>(AddonsCallbacks[CALLBACK_NODE_ON_TIMER]))(p, n, elapsed_timer.elapsed))
				block->setNodeTimer(NodeTimer(elapsed_timer.timeout, 0, elapsed_timer.position));
		}
	}
}

void ServerEnvironment::addActiveBlockModifier(ActiveBlockModifier *abm)
{
	//m_abms.emplace_back(abm); //abm engine is no longer here
	warningstream << "Called " <<FUNCTION_NAME << " with no handler!" << std::endl;
}

void ServerEnvironment::addLoadingBlockModifierDef(LoadingBlockModifierDef *lbm)
{
	m_lbm_mgr.addLBMDef(lbm);
}

std::set<v3s16> *ServerEnvironment::getForceloadedBlocks() {
	return nullptr;//&abme->ActiveBlocks.m_forceloaded_list; 
};

//Helper function for debugging
std::string get_pos_string(v3s16 pos) {
	std::string str = "("
		+ std::to_string(pos.X) + ","
		+ std::to_string(pos.Y) + ","
		+ std::to_string(pos.Z) + ")";
	return str;
}

//Queue helper for function overload
void ServerEnvironment::QueueNodeModify(int mode, v3s16 pos, const MapNode &node, bool byabm, uint16_t mapid)
{
	//If debugging and not used by ABM, then print debug info
	if (debugging && !byabm) {
		actionstream << "Queing node type " << mode << ", on " << get_pos_string(pos) << std::endl;
	}
	//MutexAutoLock autolock(QueueMutex);
	QueueMutex.lock();
	nodestoapply.emplace_back();
	auto &queue = nodestoapply.back();
	queue.node = std::move(node);
	queue.pos = pos;
	queue.from_abm = byabm;
	queue.mapid = mapid;
	QueueMutex.unlock();
}

void ServerEnvironment::QueuedNodesStep()
{
{return;}
	//only proccess data when doing changes into map
	if (!doingModifyToMap) {
		QueueMutex.lock();
		while (!nodestoapply.empty()) {
			//get node
			PerNodeQueue &NODE = nodestoapply.front();
			//do data
			uint8_t mode = NODE.type;
			uint16_t mapid = NODE.mapid;
			MapNode node = NODE.node;
			v3s16 pos = NODE.pos;
			bool abm = NODE.from_abm;
			ServerMap *map = Maps->at(mapid)->m_map;
			if (!abm) {
				if (mode == 0) //setnode
					setNode(pos, node, mapid);
				else if (mode == 1) //removenode
					removeNode(pos, mapid);
				else if (mode == 2) //swapnode
					swapNode(pos, node, mapid);
			} else {
				//Make some global variables
				const NodeDefManager *ndef = m_server->ndef();
				//Abm does execute in is own environment
				if (mode == 0) {
					doingModifyToMap = true;
					
					MapNode n_old = map->getNode(pos);
					const ContentFeatures &cf_old = ndef->get(n_old);
					// Call destructor
					if (cf_old.has_on_destruct)
						m_script->node_on_destruct(pos, n_old, mapid);
					// Replace node
					bool continue_ = true;
					if (!map->addNodeWithEvent(pos, node))
						continue_ = false;
					
					if (continue_) {
						// Update active VoxelManipulator if a mapgen thread
						map->updateVManip(pos);
						// Call post-destructor
						if (cf_old.has_after_destruct)
							m_script->node_after_destruct(pos, n_old, mapid);
						// Retrieve node content features
						// if new node is same as old, reuse old definition to prevent a lookup
						const ContentFeatures &cf_new = n_old == node ? cf_old : ndef->get(node);
						// Call constructor
						if (cf_new.has_on_construct)
							m_script->node_on_construct(pos, node, mapid);
					}
					doingModifyToMap = false;
				} else if (mode == 1) {
					doingModifyToMap = true;
					MapNode n_old = map->getNode(pos);

					// Call destructor
					if (ndef->get(n_old).has_on_destruct)
						m_script->node_on_destruct(pos, n_old, mapid);

					bool continue_ = true;
					if (!map->removeNodeWithEvent(pos))
						continue_ = false;
					
					if (continue_) {
						// Update active VoxelManipulator if a mapgen thread
						map->updateVManip(pos);

						// Call post-destructor
						if (ndef->get(n_old).has_after_destruct)
							m_script->node_after_destruct(pos, n_old, mapid);
					}
					
					doingModifyToMap = false;
				} else if (mode == 2) {
					doingModifyToMap = true;
					if (map->addNodeWithEvent(pos, node, false))
						map->updateVManip(pos);
					doingModifyToMap = false;
				}
			}
			nodestoapply.pop_front();
		}
		QueueMutex.unlock();
	}
}

bool ServerEnvironment::setNode(v3s16 p, const MapNode &n, uint16_t mapid)
{
	if (Maps->find(mapid) == Maps->end()) {
		errorstream << FUNCTION_NAME << ": Unknown map: " << mapid << std::endl;
		return false;
	}
	if (!doingModifyToMap) {
		const NodeDefManager *ndef = m_server->ndef();
		MapNode n_old = Maps->at(mapid)->m_map->getNode(p);

		const ContentFeatures &cf_old = ndef->get(n_old);

		// Call destructor
		if (cf_old.has_on_destruct)
			cf_old.on_destruct(p, n_old, mapid);
			//m_script->node_on_destruct(p, n_old, mapid);

		// Replace node
		if (!Maps->at(mapid)->m_map->addNodeWithEvent(p, n))
			return false;

		// Update active VoxelManipulator if a mapgen thread
		Maps->at(mapid)->m_map->updateVManip(p);

		// Call post-destructor
		if (cf_old.has_after_destruct)
			cf_old.after_destruct(p, n_old, mapid);
			//m_script->node_after_destruct(p, n_old, mapid);

		// Retrieve node content features
		// if new node is same as old, reuse old definition to prevent a lookup
		const ContentFeatures &cf_new = n_old == n ? cf_old : ndef->get(n);

		// Call constructor
		if (cf_new.has_on_construct)
			cf_new.on_construct(p, n_old, mapid);
			//m_script->node_on_construct(p, n, mapid);

		return true;
	} else {
		QueueNodeModify(0, p, n, false);
		warningstream << "setNode has been queued due to a function overload. POS: " << get_pos_string(p) << std::endl;
		return true;
	}
}

bool ServerEnvironment::removeNode(v3s16 p, uint16_t mapid)
{
	if (Maps->find(mapid) == Maps->end()) {
		errorstream << FUNCTION_NAME << ": Unknown map: " << mapid << std::endl;
		return false;
	}
	if (!doingModifyToMap) {
		doingModifyToMap = true;
		const NodeDefManager *ndef = m_server->ndef();
		MapNode n_old = Maps->at(mapid)->m_map->getNode(p);

		// Call destructor
		if (ndef->get(n_old).has_on_destruct)
			ndef->get(n_old).on_destruct(p, n_old, mapid);
			//m_script->node_on_destruct(p, n_old, mapid);

		// Replace with air
		// This is slightly optimized compared to addNodeWithEvent(air)
		if (!Maps->at(mapid)->m_map->removeNodeWithEvent(p))
			return false;

		// Update active VoxelManipulator if a mapgen thread
		Maps->at(mapid)->m_map->updateVManip(p);

		// Call post-destructor
		if (ndef->get(n_old).has_after_destruct)
			ndef->get(n_old).after_destruct(p, n_old, mapid);
			//m_script->node_after_destruct(p, n_old, mapid);

		// Air doesn't require constructor
		doingModifyToMap = false;
		return true;
	} else {
		MapNode node;
		QueueNodeModify(1, p, node, false);
		warningstream << "removeNode has been queued due to a function overload. POS: " << get_pos_string(p) << std::endl;
		return true;
	}
}

bool ServerEnvironment::swapNode(v3s16 p, const MapNode &n, uint16_t mapid)
{
	if (Maps->find(mapid) == Maps->end()) {
		errorstream << FUNCTION_NAME << ": Unknown map: " << mapid << std::endl;
		return false;
	}

	if (!doingModifyToMap) {
		doingModifyToMap = true;
		if (!Maps->at(mapid)->m_map->addNodeWithEvent(p, n, false))
			return false;

		// Update active VoxelManipulator if a mapgen thread
		Maps->at(mapid)->m_map->updateVManip(p);
		
		doingModifyToMap = false;
		return true;
	} else {
		QueueNodeModify(2, p, n, false);
		warningstream << "swapNode has been queued due to a function overload. POS: " << get_pos_string(p) << std::endl;
		return true;
	}
}

u8 ServerEnvironment::findSunlight(v3s16 pos, uint16_t mapid) const
{
	if (Maps->find(mapid) == Maps->end()) {
		errorstream << FUNCTION_NAME << ": Unknown map: " << mapid << std::endl;
		return 0;
	}

	// Directions for neighbouring nodes with specified order
	static const v3s16 dirs[] = {
		v3s16(-1, 0, 0), v3s16(1, 0, 0), v3s16(0, 0, -1), v3s16(0, 0, 1),
		v3s16(0, -1, 0), v3s16(0, 1, 0)
	};

	const NodeDefManager *ndef = m_server->ndef();

	// found_light remembers the highest known sunlight value at pos
	u8 found_light = 0;

	struct stack_entry {
		v3s16 pos;
		s16 dist;
	};
	std::stack<stack_entry> stack;
	stack.push({pos, 0});

	std::unordered_map<s64, s8> dists;
	dists[MapDatabase::getBlockAsInteger(pos)] = 0;

	while (!stack.empty()) {
		struct stack_entry e = stack.top();
		stack.pop();

		v3s16 currentPos = e.pos;
		s8 dist = e.dist + 1;

		for (const v3s16& off : dirs) {
			v3s16 neighborPos = currentPos + off;
			s64 neighborHash = MapDatabase::getBlockAsInteger(neighborPos);

			// Do not walk neighborPos multiple times unless the distance to the start
			// position is shorter
			auto it = dists.find(neighborHash);
			if (it != dists.end() && dist >= it->second)
				continue;

			// Position to walk
			bool is_position_ok;
			MapNode node = Maps->at(mapid)->m_map->getNode(neighborPos, &is_position_ok);
			if (!is_position_ok) {
				// This happens very rarely because the map at currentPos is loaded
				Maps->at(mapid)->m_map->emergeBlock(neighborPos, false);
				node = Maps->at(mapid)->m_map->getNode(neighborPos, &is_position_ok);
				if (!is_position_ok)
					continue;  // not generated
			}

			const ContentFeatures &def = ndef->get(node);
			if (!def.sunlight_propagates) {
				// Do not test propagation here again
				dists[neighborHash] = -1;
				continue;
			}

			// Sunlight could have come from here
			dists[neighborHash] = dist;
			u8 daylight = node.param1 & 0x0f;

			// In the special case where sunlight shines from above and thus
			// does not decrease with upwards distance, daylight is always
			// bigger than nightlight, which never reaches 15
			int possible_finlight = daylight - dist;
			if (possible_finlight <= found_light) {
				// Light from here cannot make a brighter light at currentPos than
				// found_light
				continue;
			}

			u8 nightlight = node.param1 >> 4;
			if (daylight > nightlight) {
				// Found a valid daylight
				found_light = possible_finlight;
			} else {
				// Sunlight may be darker, so walk the neighbours
				stack.push({neighborPos, dist});
			}
		}
	}
	return found_light;
}

//When using this function the objects MUST BE CLEARED ALL
void ServerEnvironment::clearObjects(ClearObjectsMode mode, uint16_t mapid)
{
	infostream << "ServerEnvironment::clearObjects(): "
		<< "Removing all active objects" << std::endl;
	auto cb_removal = [this, &mapid] (ServerActiveObject *obj, u16 id) {
		if (obj->getType() == ACTIVEOBJECT_TYPE_PLAYER)
			return false;

		if (obj->getMapId() != mapid)
			return false;

		// Delete static object if block is loaded
		deleteStaticFromBlock(obj, id, MOD_REASON_CLEAR_ALL_OBJECTS, true);

		// If known by some client, don't delete immediately
		if (obj->m_known_by_count > 0) {
			obj->markForRemoval();
			return false;
		}

		// Tell the object about removal
		obj->removingFromEnvironment();
		// Deregister in scripting api
		//m_script->removeObjectReference(obj);

		// Delete active object
		if (obj->environmentDeletes())
			delete obj;

		return true;
	};

	m_ao_manager.clear(cb_removal);

	// Get list of loaded blocks
	std::vector<v3s16> loaded_blocks;
	infostream << "ServerEnvironment::clearObjects(): Listing all loaded blocks" << std::endl;
	Maps->at(mapid)->m_map->listAllLoadedBlocks(loaded_blocks);
	infostream << "ServerEnvironment::clearObjects(): Done listing all loaded blocks: " << loaded_blocks.size()<<std::endl;

	// Get list of loadable blocks
	std::vector<v3s16> loadable_blocks;
	if (mode == CLEAR_OBJECTS_MODE_FULL) {
		infostream << "ServerEnvironment::clearObjects(): Listing all loadable blocks" << std::endl;
		Maps->at(mapid)->m_map->listAllLoadableBlocks(loadable_blocks);
		infostream << "ServerEnvironment::clearObjects(): Done listing all loadable blocks: " << loadable_blocks.size() << std::endl;
	} else {
		loadable_blocks = loaded_blocks;
	}

	actionstream << "ServerEnvironment::clearObjects(): "
		<< "Now clearing objects in " << loadable_blocks.size()
		<< " blocks" << std::endl;

	// Grab a reference on each loaded block to avoid unloading it
	for (v3s16 p : loaded_blocks) {
		MapBlock *block = Maps->at(mapid)->m_map->getBlockNoCreateNoEx(p);
		assert(block != NULL);
		block->refGrab();
	}

	// Remove objects in all loadable blocks
	u32 unload_interval = U32_MAX;
	if (mode == CLEAR_OBJECTS_MODE_FULL) {
		unload_interval = g_settings->getS32("max_clearobjects_extra_loaded_blocks");
		unload_interval = MYMAX(unload_interval, 1);
	}
	u32 report_interval = loadable_blocks.size() / 10;
	u32 num_blocks_checked = 0;
	u32 num_blocks_cleared = 0;
	u32 num_objs_cleared = 0;
	for (auto i = loadable_blocks.begin();
		i != loadable_blocks.end(); ++i) {
		v3s16 p = *i;
		MapBlock *block = Maps->at(mapid)->m_map->emergeBlock(p, false);
		if (!block) {
			errorstream << "ServerEnvironment::clearObjects(): "
				<< "Failed to emerge block " << PP(p) << std::endl;
			continue;
		}
		u32 num_stored = block->m_static_objects.m_stored.size();
		u32 num_active = block->m_static_objects.m_active.size();
		if (num_stored != 0 || num_active != 0) {
			block->m_static_objects.m_stored.clear();
			block->m_static_objects.m_active.clear();
			block->raiseModified(MOD_STATE_WRITE_NEEDED,
				MOD_REASON_CLEAR_ALL_OBJECTS);
			num_objs_cleared += num_stored + num_active;
			num_blocks_cleared++;
		}
		num_blocks_checked++;

		if (report_interval != 0 &&
			num_blocks_checked % report_interval == 0) {
			float percent = 100.0 * (float)num_blocks_checked /
				loadable_blocks.size();
			actionstream << "ServerEnvironment::clearObjects(): "
				<< "Cleared " << num_objs_cleared << " objects"
				<< " in " << num_blocks_cleared << " blocks ("
				<< percent << "%)" << std::endl;
		}
		if (num_blocks_checked % unload_interval == 0) {
			Maps->at(mapid)->m_map->unloadUnreferencedBlocks();
		}
	}
	Maps->at(mapid)->m_map->unloadUnreferencedBlocks();

	// Drop references that were added above
	for (v3s16 p : loaded_blocks) {
		MapBlock *block = Maps->at(mapid)->m_map->getBlockNoCreateNoEx(p);
		assert(block);
		block->refDrop();
	}

	m_last_clear_objects_time = m_game_time;

	actionstream << "ServerEnvironment::clearObjects(): "
		<< "Finished: Cleared " << num_objs_cleared << " objects"
		<< " in " << num_blocks_cleared << " blocks" << std::endl;
}

void ServerEnvironment::stepPlayers() {
	float dtime = 0.020f;
	bool send_recommended = true;
	QueueMutex.lock();
	auto cb_state = [this, dtime, send_recommended] (ServerActiveObject *obj) {
		if (obj->isGone())
			return;
		if (obj->getType() != ACTIVEOBJECT_TYPE_PLAYER)
			return;
		// Step object
		obj->step(dtime, send_recommended); //This might run on a different thread.
		// Read messages from object
		obj->dumpAOMessagesToQueue(m_active_object_messages);
	};
	QueueMutex.unlock();
	m_ao_manager.step(dtime, cb_state);
}

void ServerEnvironment::stepScript(float dtime) {
	//Ticks for engine
	serverenvtimer = serverenvtimer + dtime; // [serverenvtimer] = {float}
	float tick_per_step = g_settings->getFloat("ticks"); //0.02 (1.0/100) [Each tick (1,2,3,...) is 0.01, 0.02, ...]

	if (!tick_per_step)
		tick_per_step = 2.0; //default if not assigned

	/*if (serverenvtimer >= (tick_per_step/100)) {
		//Environment
		m_script->environment_Step(serverenvtimer);
		//ABME
		m_script->dostepabm();
		//Day/Night
		stepTimeOfDay(serverenvtimer);
		//reset timer
		serverenvtimer = 0.0f;
	}*/

	//Async Environment
	//m_script->stepAsync();
	//Queued nodes [Specific for function overloads and ABM on ABME]
	//QueuedNodesStep();
	//Triggers all queued ABM functions
	//TriggeringLuaOnStep();
	
	// Load the queued requests from players
	/*while (!m_m_player_queue_queuePlayerEvent.empty()) {
		PlayerSAO *sao = m_m_player_queue_queuePlayerEvent.pop();
		m_script->player_event(sao, "properties_changed");
	}
	while (!m_m_player_queue_on_punchplayer.empty()) {
		_M_M_OPP i = m_m_player_queue_on_punchplayer.pop();
		m_script->on_punchplayer(i.sao, i.puncher, i.time_from_last_punch, i.toolcap, i.dir, i.hp);
	}
	while (!m_m_player_queue_on_rightclickplayer.empty()) {
		_M_M_OR i = m_m_player_queue_on_rightclickplayer.pop();
		m_script->on_rightclickplayer(i.s, i.c);
	}
	while (!m_m_player_queue_on_hpchange.empty()) {
		_M_M_OHC i = m_m_player_queue_on_hpchange.pop();
		m_script->on_player_hpchange(i.sao, i.HP, i.PHCR);
	}*/
}

//BEGIN UNUSED

// Some queue''rs
void ServerEnvironment::queueOnPunchPlayer(PlayerSAO *playersao, ServerActiveObject *puncher, float time_from_last_punch, const ToolCapabilities *toolcap, const v3f dir, u16 hp) {
	_M_M_OPP i;
	i.sao = playersao;
	i.puncher = puncher;
	i.time_from_last_punch = time_from_last_punch;
	//i.toolcap = toolcap;
	i.dir = dir;
	i.hp = hp;
	m_m_player_queue_on_punchplayer.push(i);
}
void ServerEnvironment::queuePlayerEvent(PlayerSAO *s) {
	m_m_player_queue_queuePlayerEvent.push(s);
}
void ServerEnvironment::queueOnRightClickPlayer(PlayerSAO *p, ServerActiveObject *sao) {
	_M_M_OR i;
	i.s = p;
	i.c = sao;
	m_m_player_queue_on_rightclickplayer.push(i);
}
void ServerEnvironment::queueOnChangeHP(PlayerSAO *player, u16 hp, const PlayerHPChangeReason reason) { // uhm. . . u16?
	//_M_M_OHC i;
	//i.sao = player;
	//i.PHCR = reason;
	//i.HP = hp;
}

//END UNUSED

void ServerEnvironment::step(float dtime)
{
	// Update this one
	// NOTE: This is kind of funny on a singleplayer game, but doesn't
	// really matter that much.
	static thread_local const float server_step = 0.09;
	m_recommended_send_interval = server_step;

	/*
		Increment game time
	*/
	{
		m_game_time_fraction_counter += dtime;
		u32 inc_i = (u32)m_game_time_fraction_counter;
		m_game_time += inc_i;
		m_game_time_fraction_counter -= (float)inc_i;
	}
	/*
		Manage active block list
	*/
	if (m_active_blocks_management_interval.step(dtime, m_cache_active_block_mgmt_interval)) {
		/*
			Get player block positions
		*/
		std::vector<PlayerSAO*> players;
		for (auto i = PnameToID.begin(); i != PnameToID.end(); i++) {
			RemotePlayer *player = getPlayer(i->second);
			// Ignore disconnected players
			if (!m_server->ServersNetworkObject->AreSlave) {
				if (!player)
					break; //May broken list
				if (player->getPeerId() == PEER_ID_INEXISTENT)
					continue;
				if (player->OnServer->IsApplied)
					continue;
			}
			
			PlayerSAO *playersao = player->getPlayerSAO();
			if (!playersao) //means if the player was initializing or exiting from the game
				continue;
			assert(playersao);

			players.push_back(playersao);
		}

		/*
			Update list of active blocks, collecting changes
		*/
		// use active_object_send_range_blocks since that is max distance
		// for active objects sent the client anyway
		/*static thread_local const s16 aor = g_settings->getS16("active_object_send_range_blocks"); //active_object_range
		static thread_local const s16 abr = g_settings->getS16("active_block_range"); //active_block_range
		std::set<v3s16> blocks_removed;
		std::set<v3s16> blocks_added;*/
		//m_active_blocks.update(players, active_block_range, active_object_range,
		//	blocks_removed, blocks_added);

		//OBJ:::SET_HERE_UPDATE_FUNCTION()
		
		//abme->ActiveBlocks.update(players, abr, aor, blocks_removed, blocks_added);

		/*
			Handle removed blocks
		*/
		
		//TODO: Make ABM(E) be in builtin - Addons.
		
		// Convert active objects that are no more in active blocks to static
		deactivateFarObjects(false);

		/*for (const v3s16 &p: blocks_removed) {
			MapBlock *block = Maps->at(0)->m_map->getBlockNoCreateNoEx(p);
			if (!block)
				continue;

			// Set current time as timestamp (and let it set ChangedFlag)
			block->setTimestamp(m_game_time);
		}

		/*
			Handle added blocks
		

		for (const v3s16 &p: blocks_added) {
			MapBlock *block = Maps->at(0)->m_map->getBlockOrEmerge(p);
			if (!block) {
				abme->delete_block(p);//m_active_blocks.m_list.erase(p);
				abme->delete_block_abmlist(p);//m_active_blocks.m_abm_list.erase(p);
				continue;
			}

			activateBlock(block, 0);
		}*/
	}

	/*
		Mess around in active blocks
	
	if (m_active_blocks_nodemetadata_interval.step(dtime, m_cache_nodetimer_interval)) {

		float dtime = m_cache_nodetimer_interval;

		if (abme == nullptr) {
			throw LuaError("Blocked possible Segmentation Fault:\nActive Block Modifier Engine+Threading (ABME+T) is nullptr\nMaybe is a mod in the engine making problems. [At m_active_blocks_nodemetadata_interval.step(., .)]");
		}

		//NOTE: Each map should have his own ActiveBlockManagerEngine
		for (const v3s16 &p: abme->ActiveBlocks.m_list) {
			MapBlock *block = Maps->at(0)->m_map->getBlockNoCreateNoEx(p);
			if (!block)
				continue;

			// Reset block usage timer
			block->resetUsageTimer();

			// Set current time as timestamp
			block->setTimestampNoChangedFlag(m_game_time);
			// If time has changed much from the one on disk,
			// set block to be saved when it is unloaded
			if(block->getTimestamp() > block->getDiskTimestamp() + 60)
				block->raiseModified(MOD_STATE_WRITE_AT_UNLOAD,
					MOD_REASON_BLOCK_EXPIRED);

			// Run node timers
			std::vector<NodeTimer> elapsed_timers = block->m_node_timers.step(dtime);
			if (!elapsed_timers.empty()) {
				MapNode n;
				v3s16 p2;
				for (const NodeTimer &elapsed_timer: elapsed_timers) {
					n = block->getNodeNoEx(elapsed_timer.position);
					p2 = elapsed_timer.position + block->getPosRelative();
					//if (m_script->node_on_timer(p2, n, elapsed_timer.elapsed)) {
					if ((reinterpret_cast<bool(*)(v3s16, MapNode, float)>(AddonsCallbacks[CALLBACK_NODE_ON_TIMER]))(p2, n, elapsed_timer.elapsed)) {
						block->setNodeTimer(NodeTimer(elapsed_timer.timeout, 0, elapsed_timer.position));
					}
				}
			}
		}
	}
	//m_active_block_modifier_interval.step(dtime, m_cache_abm_interval)
	m_abm_timer += dtime;
	if (g_settings->getBool("do_abm") && m_abm_timer >= m_cache_abm_interval) {
		verbosestream << "On step [" << m_cache_abm_interval << ", " << m_abm_timer << "]" << std::endl;
		abme->step();
		m_abm_timer = 0.0f;
	}*/
	

	/*
		Step active objects
	*/
	{
		// This helps the objects to send data at the same time
		bool send_recommended = true;
		m_send_recommended_timer += dtime;
		if (m_send_recommended_timer > getSendRecommendedInterval()) {
			m_send_recommended_timer -= getSendRecommendedInterval();
			send_recommended = true;
		}
	
		QueueMutex.lock();
		auto cb_state = [this, dtime, send_recommended] (ServerActiveObject *obj) {
			if (obj->isGone())
				return;
			if (obj->getType() == ACTIVEOBJECT_TYPE_PLAYER) //Players will need to be updated in other thread "Players"
				return;
			// Step object
			obj->step(dtime, send_recommended); //This might run on a different thread.
			// Read messages from object
			obj->dumpAOMessagesToQueue(m_active_object_messages);
		};
		QueueMutex.unlock();
		m_ao_manager.step(dtime, cb_state);
	}

	/*
		Manage active objects
	*/
	if (m_object_management_interval.step(dtime, 0.5)) {
		removeRemovedObjects();
	}

	/*
		Manage particle spawner expiration
	*/
	if (m_particle_management_interval.step(dtime, 1.0)) {
		for (std::unordered_map<u32, float>::iterator i = m_particle_spawners.begin();
			i != m_particle_spawners.end(); ) {
			//non expiring spawners
			if (i->second == PARTICLE_SPAWNER_NO_EXPIRY) {
				++i;
				continue;
			}

			i->second -= 1.0f;
			if (i->second <= 0.f)
				m_particle_spawners.erase(i++);
			else
				++i;
		}
	}

	// Send outdated detached inventories
	m_server->sendDetachedInventories(PEER_ID_INEXISTENT, true);
}

/////////////////////////

void ServerEnvironment::TriggeringLuaOnStep() {
	//Might has to copy table
	//std::deque<TriggeringLuaQueue> triggeringluatoapply_;
	while (!triggeringluatoapply.empty()) {
		//main
		QueueTriggerMutex.lock();
		TriggeringLuaQueue &DATA = triggeringluatoapply.front();
		triggeringluatoapply.pop_front();
		QueueTriggerMutex.unlock();
		//query
		MapNode n = DATA.node;
		v3s16 p = DATA.pos;
		u32 aoc  = DATA.aoc;
		u32 aocw = DATA.aocw;
		ActiveBlockModifier *abm = DATA.abm;
		//call
		//abm->trigger(this, p, n); //as is, does not exist anymore.
		if (!abm) {
			errorstream << "Got nullptr and not abm" << std::endl;
			triggeringluatoapply.pop_front();
			continue;
		}
		abm->trigger(this, p, n, aoc, aocw);
		//pop
		
	}
}

void ServerEnvironment::QueueTriggerForABM(ActiveBlockModifier *abm, v3s16 pos, MapNode node, u32 aoc, u32 aocw) {
	//MutexAutoLock autolock(QueueTriggerMutex);
	QueueTriggerMutex.lock();
	triggeringluatoapply.emplace_back();
	auto &d = triggeringluatoapply.back();
	d.node = node ;
	d.pos  = pos  ;
	d.aoc  = aoc  ;
	d.aocw = aocw ;
	d.abm  = std::move(abm);
	QueueTriggerMutex.unlock();
}

/////////////////////////

u32 ServerEnvironment::addParticleSpawner(float exptime)
{
	// Timers with lifetime 0 do not expire
	float time = exptime > 0.f ? exptime : PARTICLE_SPAWNER_NO_EXPIRY;

	u32 id = 0;
	for (;;) { // look for unused particlespawner id
		id++;
		std::unordered_map<u32, float>::iterator f = m_particle_spawners.find(id);
		if (f == m_particle_spawners.end()) {
			m_particle_spawners[id] = time;
			break;
		}
	}
	return id;
}

u32 ServerEnvironment::addParticleSpawner(float exptime, u16 attached_id)
{
	u32 id = addParticleSpawner(exptime);
	m_particle_spawner_attachments[id] = attached_id;
	if (ServerActiveObject *obj = getActiveObject(attached_id)) {
		obj->attachParticleSpawner(id);
	}
	return id;
}

void ServerEnvironment::deleteParticleSpawner(u32 id, bool remove_from_object)
{
	m_particle_spawners.erase(id);
	const auto &it = m_particle_spawner_attachments.find(id);
	if (it != m_particle_spawner_attachments.end()) {
		u16 obj_id = it->second;
		ServerActiveObject *sao = getActiveObject(obj_id);
		if (sao != NULL && remove_from_object) {
			sao->detachParticleSpawner(id);
		}
		m_particle_spawner_attachments.erase(id);
	}
}

u16 ServerEnvironment::addActiveObject(ServerActiveObject *object, uint16_t mapid)
{
	assert(object);	// Pre-condition
	m_added_objects++;
	u16 id = addActiveObjectRaw(object, true, 0, mapid);
	return id;
}

/*
	Finds out what new objects have been added to
	inside a radius around a position
*/
void ServerEnvironment::getAddedActiveObjects(PlayerSAO *playersao, s16 radius, s16
player_radius, std::set<u16> &current_objects, std::queue<u16> &added_objects)
{
	f32 radius_f = radius * BS;
	f32 player_radius_f = player_radius * BS;

	if (player_radius_f < 0.0f)
		player_radius_f = 0.0f;

	m_ao_manager.getAddedActiveObjectsAroundPos(playersao->getBasePosition(),
radius_f, player_radius_f, current_objects, added_objects, playersao->getPlayer()->m_map_id);
}

/*
	Finds out what objects have been removed from
	inside a radius around a position
*/
void ServerEnvironment::getRemovedActiveObjects(PlayerSAO *playersao, s16 radius,
	s16 player_radius,
	std::set<u16> &current_objects,
	std::queue<u16> &removed_objects)
{
	f32 radius_f = radius * BS;
	f32 player_radius_f = player_radius * BS;

	if (player_radius_f < 0)
		player_radius_f = 0;
	/*
		Go through current_objects; object is removed if:
		- object is not found in m_active_objects (this is actually an
		  error condition; objects should be removed only after all clients
		  have been informed about removal), or
		- object is to be removed or deactivated, or
		- object is too far away
	*/
	for (u16 id : current_objects) {
		ServerActiveObject *object = getActiveObject(id);

		if (object == NULL) {
			infostream << "ServerEnvironment::getRemovedActiveObjects():"
				<< " object in current_objects is NULL" << std::endl;
			removed_objects.push(id);
			continue;
		}

		if (object->getMapId() != playersao->getPlayer()->m_map_id)
			continue;

		if (object->isGone()) {
			removed_objects.push(id);
			continue;
		}

		f32 distance_f = object->getBasePosition().getDistanceFrom(playersao->getBasePosition());
		if (object->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			if (distance_f <= player_radius_f || player_radius_f == 0)
				continue;
		} else if (distance_f <= radius_f)
			continue;

		// Object is no longer visible
		removed_objects.push(id);
	}
}

void ServerEnvironment::setStaticForActiveObjectsInBlock(
	v3s16 blockpos, bool static_exists, v3s16 static_block)
{
	MapBlock *block = m_map->getBlockNoCreateNoEx(blockpos);
	if (!block)
		return;

	for (auto &so_it : block->m_static_objects.m_active) {
		// Get the ServerActiveObject counterpart to this StaticObject
		ServerActiveObject *sao = m_ao_manager.getActiveObject(so_it.first);
		if (!sao) {
			// If this ever happens, there must be some kind of nasty bug.
			errorstream << "ServerEnvironment::setStaticForObjectsInBlock(): "
				"Object from MapBlock::m_static_objects::m_active not found "
				"in m_active_objects";
			continue;
		}

		sao->m_static_exists = static_exists;
		sao->m_static_block  = static_block;
	}
}

bool ServerEnvironment::getActiveObjectMessage(ActiveObjectMessage *dest)
{
	QueueMutex.lock();
	if(m_active_object_messages.empty()) {
		QueueMutex.unlock();
		return false;
	}

	*dest = std::move(m_active_object_messages.front());
	m_active_object_messages.pop();
	QueueMutex.unlock();
	return true;
}

void ServerEnvironment::getSelectedActiveObjects(
	const core::line3d<f32> &shootline_on_map,
	std::vector<PointedThing> &objects)
{
	std::vector<ServerActiveObject *> objs;
	getObjectsInsideRadius(objs, shootline_on_map.start,
		shootline_on_map.getLength() + 10.0f, nullptr);
	const v3f line_vector = shootline_on_map.getVector();

	for (auto obj : objs) {
		if (obj->isGone())
			continue;
		aabb3f selection_box;
		if (!obj->getSelectionBox(&selection_box))
			continue;

		v3f pos = obj->getBasePosition();

		aabb3f offsetted_box(selection_box.MinEdge + pos,
			selection_box.MaxEdge + pos);

		v3f current_intersection;
		v3s16 current_normal;
		if (boxLineCollision(offsetted_box, shootline_on_map.start, line_vector,
				&current_intersection, &current_normal)) {
			objects.emplace_back(
				(s16) obj->getId(), current_intersection, current_normal,
				(current_intersection - shootline_on_map.start).getLengthSQ());
		}
	}
}

/*
	************ Private methods *************
*/

u16 ServerEnvironment::addActiveObjectRaw(ServerActiveObject *object, bool set_changed, u32 dtime_s, uint16_t mapid)
{
	if (!m_ao_manager.registerObject(object)) {
		return 0;
	}

	// Register reference in scripting api (must be done before post-init)
	//m_script->addObjectReference(object);
	// Post-initialize object
	object->addedToEnvironment(dtime_s);
	object->setMapID(mapid);

	// Add static data to block
	if (object->isStaticAllowed()) {
		// Add static object to active static list of the block
		v3f objectpos = object->getBasePosition();
		StaticObject s_obj(object, objectpos);
		// Add to the block where the object is located in
		v3s16 blockpos = getNodeBlockPos(floatToInt(objectpos, BS));
		MapBlock *block = Maps->at(mapid)->m_map->emergeBlock(blockpos);
		if(block){
			block->m_static_objects.m_active[object->getId()] = s_obj;
			object->m_static_exists = true;
			object->m_static_block = blockpos;

			if(set_changed)
				block->raiseModified(MOD_STATE_WRITE_NEEDED,
					MOD_REASON_ADD_ACTIVE_OBJECT_RAW);
		} else {
			v3s16 p = floatToInt(objectpos, BS);
			errorstream<<"ServerEnvironment::addActiveObjectRaw(): "
				<<"could not emerge block for storing id="<<object->getId()
				<<" statically (pos="<<PP(p)<<")"<<std::endl;
		}
	}

	return object->getId();
}

/*
	Remove objects that satisfy (isGone() && m_known_by_count==0)
*/
void ServerEnvironment::removeRemovedObjects()
{
	auto clear_cb = [this] (ServerActiveObject *obj, u16 id) {
		// This shouldn't happen but check it
		if (!obj) {
			errorstream << "ServerEnvironment::removeRemovedObjects(): NULL object found. id=" << id << std::endl;
			return true;
		}

		/*
			We will handle objects marked for removal or deactivation
		*/
		if (!obj->isGone())
			return false;

		/*
			Delete static data from block if removed
		*/
		if (obj->isPendingRemoval())
			deleteStaticFromBlock(obj, id, MOD_REASON_REMOVE_OBJECTS_REMOVE, false);

		// If still known by clients, don't actually remove. On some future
		// invocation this will be 0, which is when removal will continue.
		if(obj->m_known_by_count > 0)
			return false;

		/*
			Move static data from active to stored if deactivated
		*/
		if (!obj->isPendingRemoval() && obj->m_static_exists) {
			MapBlock *block = m_map->emergeBlock(obj->m_static_block, false);
			if (block) {
				const auto i = block->m_static_objects.m_active.find(id);
				if (i != block->m_static_objects.m_active.end()) {
					block->m_static_objects.m_stored.push_back(i->second);
					block->m_static_objects.m_active.erase(id);
					block->raiseModified(MOD_STATE_WRITE_NEEDED,
						MOD_REASON_REMOVE_OBJECTS_DEACTIVATE);
				} else {
					warningstream << "ServerEnvironment::removeRemovedObjects(): "
							<< "id=" << id << " m_static_exists=true but "
							<< "static data doesn't actually exist in "
							<< PP(obj->m_static_block) << std::endl;
				}
			} else {
				infostream << "Failed to emerge block from which an object to "
						<< "be deactivated was loaded from. id=" << id << std::endl;
			}
		}

		// Tell the object about removal
		obj->removingFromEnvironment();
		// Deregister in scripting api
		//m_script->removeObjectReference(obj);

		// Delete
		if (obj->environmentDeletes())
			delete obj;

		return true;
	};

	m_ao_manager.clear(clear_cb);
}

static void print_hexdump(std::ostream &o, const std::string &data)
{
	const int linelength = 16;
	for(int l=0; ; l++){
		int i0 = linelength * l;
		bool at_end = false;
		int thislinelength = linelength;
		if(i0 + thislinelength > (int)data.size()){
			thislinelength = data.size() - i0;
			at_end = true;
		}
		for(int di=0; di<linelength; di++){
			int i = i0 + di;
			char buf[4];
			if(di<thislinelength)
				porting::mt_snprintf(buf, sizeof(buf), "%.2x ", data[i]);
			else
				porting::mt_snprintf(buf, sizeof(buf), "   ");
			o<<buf;
		}
		o<<" ";
		for(int di=0; di<thislinelength; di++){
			int i = i0 + di;
			if(data[i] >= 32)
				o<<data[i];
			else
				o<<".";
		}
		o<<std::endl;
		if(at_end)
			break;
	}
}

ServerActiveObject* ServerEnvironment::createSAO(ActiveObjectType type, v3f pos, const std::string
&data, uint16_t mapid)
{
	switch (type) {
		case ACTIVEOBJECT_TYPE_LUAENTITY:
			return new LuaEntitySAO(this, pos, data, mapid);
		default:
			warningstream << "ServerActiveObject: No factory for type=" << type << std::endl;
	}
	return nullptr;
}

/*
	Convert stored objects from blocks near the players to active.
*/
void ServerEnvironment::activateObjects(MapBlock *block, u32 dtime_s, uint16_t mapid)
{
	if(block == NULL)
		return;

	// Ignore if no stored objects (to not set changed flag)
	if(block->m_static_objects.m_stored.empty())
		return;

	verbosestream<<"ServerEnvironment::activateObjects(): "
		<<"activating objects of block "<<PP(block->getPos())
		<<" ("<<block->m_static_objects.m_stored.size()
		<<" objects)"<<std::endl;
	bool large_amount = (block->m_static_objects.m_stored.size() > g_settings->getU16("max_objects_per_block"));
	if (large_amount) {
		warningstream<<"suspiciously large amount of objects detected: "
			<<block->m_static_objects.m_stored.size()<<" in "
			<<PP(block->getPos())
			<<"; removing all of them."<<std::endl;
		// Clear stored list
		block->m_static_objects.m_stored.clear();
		block->raiseModified(MOD_STATE_WRITE_NEEDED,
			MOD_REASON_TOO_MANY_OBJECTS);
		return;
	}

	// Activate stored objects
	std::vector<StaticObject> new_stored;
	for (const StaticObject &s_obj : block->m_static_objects.m_stored) {
		// Create an active object from the data
		ServerActiveObject *obj = createSAO((ActiveObjectType) s_obj.type, s_obj.pos, s_obj.data,
s_obj.mapid);
		// If couldn't create object, store static data back.
		if (!obj) {
			errorstream<<"ServerEnvironment::activateObjects(): failed to create active object from static object " <<"in block " << PP(s_obj.pos/BS) << " type=" << (int)s_obj.type << " data:" << std::endl;
			print_hexdump(verbosestream, s_obj.data);
			new_stored.push_back(s_obj);
			continue;
		}
		verbosestream<<"ServerEnvironment::activateObjects(): activated static object pos="<<PP(s_obj.pos/BS)<<" type="<<(int)s_obj.type<<std::endl;
		// This will also add the object to the active static list
		addActiveObjectRaw(obj, false, dtime_s, mapid);
	}

	// Clear stored list
	block->m_static_objects.m_stored.clear();
	// Add leftover failed stuff to stored list
	for (const StaticObject &s_obj : new_stored) {
		block->m_static_objects.m_stored.push_back(s_obj);
	}

	/*
		Note: Block hasn't really been modified here.
		The objects have just been activated and moved from the stored
		static list to the active static list.
		As such, the block is essentially the same.
		Thus, do not call block->raiseModified(MOD_STATE_WRITE_NEEDED).
		Otherwise there would be a huge amount of unnecessary I/O.
	*/
}

/*
	Convert objects that are not standing inside active blocks to static.

	If m_known_by_count != 0, active object is not deleted, but static
	data is still updated.

	If force_delete is set, active object is deleted nevertheless. It
	shall only be set so in the destructor of the environment.

	If block wasn't generated (not in memory or on disk),
*/
void ServerEnvironment::deactivateFarObjects(bool _force_delete)
{
	/*auto cb_deactivate = [this, _force_delete] (ServerActiveObject *obj, u16 id) {
		// force_delete might be overriden per object
		bool force_delete = _force_delete;

		// Do not deactivate if disallowed
		if (!force_delete && !obj->shouldUnload())
			return false;

		// removeRemovedObjects() is responsible for these
		if (!force_delete && obj->isGone())
			return false;

		const v3f &objectpos = obj->getBasePosition();

		// The block in which the object resides in
		v3s16 blockpos_o = getNodeBlockPos(floatToInt(objectpos, BS));

		// If object's static data is stored in a deactivated block and object
		// is actually located in an active block, re-save to the block in
		// which the object is actually located in.
		
		if (abme == nullptr) {
			throw LuaError("Blocked possible Segmentation Fault:\nActive Block Modifier Engine+Threading (ABME+T) is nullptr\nMaybe is a mod in the engine making problems. [At __::deactivateFarObjects()[Line 28 in function C++]]");
		}
		
		if (!force_delete && obj->m_static_exists && !abme->ActiveBlocks.contains(obj->m_static_block) && abme->ActiveBlocks.contains(blockpos_o)) {

			// Delete from block where object was located
			deleteStaticFromBlock(obj, id, MOD_REASON_STATIC_DATA_REMOVED, false);

			StaticObject s_obj(obj, objectpos);
			// Save to block where object is located
			saveStaticToBlock(blockpos_o, id, obj, s_obj, MOD_REASON_STATIC_DATA_ADDED);

			return false;
		}

		// If block is still active, don't remove
		bool still_active = obj->isStaticAllowed() ? abme->ActiveBlocks.contains(blockpos_o) : getMap().getBlockNoCreateNoEx(blockpos_o) != nullptr;
		
		if (!force_delete && still_active)
			return false;

		verbosestream << "ServerEnvironment::deactivateFarObjects(): "
					  << "deactivating object id=" << id << " on inactive block "
					  << PP(blockpos_o) << std::endl;

		// If known by some client, don't immediately delete.
		bool pending_delete = (obj->m_known_by_count > 0 && !force_delete);

		/*
			Update the static data
		
		if (obj->isStaticAllowed()) {
			// Create new static object
			StaticObject s_obj(obj, objectpos);

			bool stays_in_same_block = false;
			bool data_changed = true;

			// Check if static data has changed considerably
			if (obj->m_static_exists) {
				if (obj->m_static_block == blockpos_o)
					stays_in_same_block = true;

				MapBlock *block = m_map->emergeBlock(obj->m_static_block, false);

				if (block) {
					const auto n = block->m_static_objects.m_active.find(id);
					if (n != block->m_static_objects.m_active.end()) {
						StaticObject static_old = n->second;

						float save_movem = obj->getMinimumSavedMovement();

						if (static_old.data == s_obj.data &&
							(static_old.pos - objectpos).getLength() < save_movem)
							data_changed = false;
					} else {
						warningstream << "ServerEnvironment::deactivateFarObjects(): "
								<< "id=" << id << " m_static_exists=true but "
								<< "static data doesn't actually exist in "
								<< PP(obj->m_static_block) << std::endl;
					}
				}
			}

			/*
				While changes are always saved, blocks are only marked as modified
				if the object has moved or different staticdata. (see above)
			
			bool shall_be_written = (!stays_in_same_block || data_changed);
			u32 reason = shall_be_written ? MOD_REASON_STATIC_DATA_CHANGED : MOD_REASON_UNKNOWN;

			// Delete old static object
			deleteStaticFromBlock(obj, id, reason, false);

			// Add to the block where the object is located in
			v3s16 blockpos = getNodeBlockPos(floatToInt(objectpos, BS));
			u16 store_id = pending_delete ? id : 0;
			if (!saveStaticToBlock(blockpos, store_id, obj, s_obj, reason))
				force_delete = true;
		}

		// Regardless of what happens to the object at this point, deactivate it first.
		// This ensures that LuaEntity on_deactivate is always called.
		obj->markForDeactivation();

		/*
			If known by some client, set pending deactivation.
			Otherwise delete it immediately.
		
		if (pending_delete && !force_delete) {
			verbosestream << "ServerEnvironment::deactivateFarObjects(): "
						  << "object id=" << id << " is known by clients"
						  << "; not deleting yet" << std::endl;

			return false;
		}

		verbosestream << "ServerEnvironment::deactivateFarObjects(): "
					  << "object id=" << id << " is not known by clients"
					  << "; deleting" << std::endl;

		// Tell the object about removal
		obj->removingFromEnvironment();
		// Deregister in scripting api
		//m_script->removeObjectReference(obj);

		// Delete active object
		if (obj->environmentDeletes())
			delete obj;

		return true;
	};

	m_ao_manager.clear(cb_deactivate);*/
}

void ServerEnvironment::deleteStaticFromBlock(
		ServerActiveObject *obj, u16 id, u32 mod_reason, bool no_emerge)
{
	if (!obj->m_static_exists)
		return;

	MapBlock *block;
	if (no_emerge)
		block = m_map->getBlockNoCreateNoEx(obj->m_static_block);
	else
		block = m_map->emergeBlock(obj->m_static_block, false);
	if (!block) {
		if (!no_emerge)
			errorstream << "ServerEnv: Failed to emerge block " << PP(obj->m_static_block)
					<< " when deleting static data of object from it. id=" << id << std::endl;
		return;
	}

	block->m_static_objects.remove(id);
	if (mod_reason != MOD_REASON_UNKNOWN) // Do not mark as modified if requested
		block->raiseModified(MOD_STATE_WRITE_NEEDED, mod_reason);

	obj->m_static_exists = false;
}

bool ServerEnvironment::saveStaticToBlock(v3s16 blockpos, u16 store_id, ServerActiveObject
*obj, const StaticObject &s_obj, u32 mod_reason) {
	MapBlock *block = nullptr;
	try {
		block = m_map->emergeBlock(blockpos);
	} catch (InvalidPositionException &e) {
		// Handled via NULL pointer
		// NOTE: emergeBlock's failure is usually determined by it
		//       actually returning NULL
	}

	if (!block) {
		errorstream << "ServerEnv: Failed to emerge block " << PP(obj->m_static_block)
				<< " when saving static data of object to it. id=" << store_id << std::endl;
		return false;
	}
	if (block->m_static_objects.m_stored.size() >= g_settings->getU16("max_objects_per_block")) {
		warningstream << "ServerEnv: Trying to store id = " << store_id
				<< " statically but block " << PP(blockpos)
				<< " already contains "
				<< block->m_static_objects.m_stored.size()
				<< " objects." << std::endl;
		return false;
	}

	block->m_static_objects.insert(store_id, s_obj);
	if (mod_reason != MOD_REASON_UNKNOWN) // Do not mark as modified if requested
		block->raiseModified(MOD_STATE_WRITE_NEEDED, mod_reason);

	obj->m_static_exists = true;
	obj->m_static_block = blockpos;

	return true;
}

PlayerDatabase *ServerEnvironment::openPlayerDatabase(const std::string &name,
		const std::string &savedir, const Settings &conf)
{
#if USE_SQLITE
	if (name == "sqlite3")
		return new PlayerDatabaseSQLite3(savedir);
#endif

	if (name == "dummy")
		return new Database_Dummy();

#if USE_POSTGRESQL
	if (name == "postgresql") {
		std::string connect_string;
		conf.getNoEx("pgsql_player_connection", connect_string);
		return new PlayerDatabasePostgreSQL(connect_string);
	}
#endif

#if USE_LEVELDB
	if (name == "leveldb")
		return new PlayerDatabaseLevelDB(savedir);
#endif

	if (name == "files")
		return new PlayerDatabaseFiles(savedir + DIR_DELIM + "players");

	throw ModError(std::string("Database backend ") + name + " not supported.");
}

bool ServerEnvironment::migratePlayersDatabase(const GameParams &game_params,
		const Settings &cmd_args)
{
	std::string migrate_to = cmd_args.get("migrate-players");
	Settings world_mt;
	std::string world_mt_path = game_params.world_path + DIR_DELIM + "world.mt";
	if (!world_mt.readConfigFile(world_mt_path.c_str())) {
		errorstream << "Cannot read world.mt!" << std::endl;
		return false;
	}

	if (!world_mt.exists("player_backend")) {
		errorstream << "Please specify your current backend in world.mt:"
			<< std::endl
			<< "	player_backend = {files|sqlite3|leveldb|postgresql}"
			<< std::endl;
		return false;
	}

	std::string backend = world_mt.get("player_backend");
	if (backend == migrate_to) {
		errorstream << "Cannot migrate: new backend is same"
			<< " as the old one" << std::endl;
		return false;
	}

	const std::string players_backup_path = game_params.world_path + DIR_DELIM
		+ "players.bak";

	if (backend == "files") {
		// Create backup directory
		fs::CreateDir(players_backup_path);
	}

	try {
		PlayerDatabase *srcdb = ServerEnvironment::openPlayerDatabase(backend,
			game_params.world_path, world_mt);
		PlayerDatabase *dstdb = ServerEnvironment::openPlayerDatabase(migrate_to,
			game_params.world_path, world_mt);

		std::vector<std::string> player_list;
		srcdb->listPlayers(player_list);
		for (std::vector<std::string>::const_iterator it = player_list.begin();
			it != player_list.end(); ++it) {
			actionstream << "Migrating player " << it->c_str() << std::endl;
			RemotePlayer player(it->c_str(), NULL);
			PlayerSAO playerSAO(NULL, &player, 15000, false);

			srcdb->loadPlayer(&player, &playerSAO);

			playerSAO.finalize(&player, std::set<std::string>());
			player.setPlayerSAO(&playerSAO);

			dstdb->savePlayer(&player);

			// For files source, move player files to backup dir
			if (backend == "files") {
				fs::Rename(
					game_params.world_path + DIR_DELIM + "players" + DIR_DELIM + (*it),
					players_backup_path + DIR_DELIM + (*it));
			}
		}

		actionstream << "Successfully migrated " << player_list.size() << " players"
			<< std::endl;
		world_mt.set("player_backend", migrate_to);
		if (!world_mt.updateConfigFile(world_mt_path.c_str()))
			errorstream << "Failed to update world.mt!" << std::endl;
		else
			actionstream << "world.mt updated" << std::endl;

		// When migration is finished from file backend, remove players directory if empty
		if (backend == "files") {
			fs::DeleteSingleFileOrEmptyDirectory(game_params.world_path + DIR_DELIM
				+ "players");
		}

		delete srcdb;
		delete dstdb;

	} catch (BaseException &e) {
		errorstream << "An error occurred during migration: " << e.what() << std::endl;
		return false;
	}
	return true;
}

AuthDatabase *ServerEnvironment::openAuthDatabase(
		const std::string &name, const std::string &savedir, const Settings &conf)
{
#if USE_SQLITE
	if (name == "sqlite3")
		return new AuthDatabaseSQLite3(savedir);
#endif

#if USE_POSTGRESQL
	if (name == "postgresql") {
		std::string connect_string;
		conf.getNoEx("pgsql_auth_connection", connect_string);
		return new AuthDatabasePostgreSQL(connect_string);
	}
#endif

	if (name == "files")
		return new AuthDatabaseFiles(savedir);

#if USE_LEVELDB
	if (name == "leveldb")
		return new AuthDatabaseLevelDB(savedir);
#endif

	throw ModError(std::string("Database backend ") + name + " not supported.");
}

bool ServerEnvironment::migrateAuthDatabase(
		const GameParams &game_params, const Settings &cmd_args)
{
	std::string migrate_to = cmd_args.get("migrate-auth");
	Settings world_mt;
	std::string world_mt_path = game_params.world_path + DIR_DELIM + "world.mt";
	if (!world_mt.readConfigFile(world_mt_path.c_str())) {
		errorstream << "Cannot read world.mt!" << std::endl;
		return false;
	}

	std::string backend = "files";
	if (world_mt.exists("auth_backend"))
		backend = world_mt.get("auth_backend");
	else
		warningstream << "No auth_backend found in world.mt, "
				"assuming \"files\"." << std::endl;

	if (backend == migrate_to) {
		errorstream << "Cannot migrate: new backend is same"
				<< " as the old one" << std::endl;
		return false;
	}

	try {
		const std::unique_ptr<AuthDatabase> srcdb(ServerEnvironment::openAuthDatabase(
				backend, game_params.world_path, world_mt));
		const std::unique_ptr<AuthDatabase> dstdb(ServerEnvironment::openAuthDatabase(
				migrate_to, game_params.world_path, world_mt));

		std::vector<std::string> names_list;
		srcdb->listNames(names_list);
		for (const std::string &name : names_list) {
			actionstream << "Migrating auth entry for " << name << std::endl;
			bool success;
			AuthEntry authEntry;
			success = srcdb->getAuth(name, authEntry);
			success = success && dstdb->createAuth(authEntry);
			if (!success)
				errorstream << "Failed to migrate " << name << std::endl;
		}

		actionstream << "Successfully migrated " << names_list.size()
				<< " auth entries" << std::endl;
		world_mt.set("auth_backend", migrate_to);
		if (!world_mt.updateConfigFile(world_mt_path.c_str()))
			errorstream << "Failed to update world.mt!" << std::endl;
		else
			actionstream << "world.mt updated" << std::endl;

		if (backend == "files") {
			// special-case files migration:
			// move auth.txt to auth.txt.bak if possible
			std::string auth_txt_path =
					game_params.world_path + DIR_DELIM + "auth.txt";
			std::string auth_bak_path = auth_txt_path + ".bak";
			if (!fs::PathExists(auth_bak_path))
				if (fs::Rename(auth_txt_path, auth_bak_path))
					actionstream << "Renamed auth.txt to auth.txt.bak"
							<< std::endl;
				else
					errorstream << "Could not rename auth.txt to "
							"auth.txt.bak" << std::endl;
			else
				warningstream << "auth.txt.bak already exists, auth.txt "
						"not renamed" << std::endl;
		}

	} catch (BaseException &e) {
		errorstream << "An error occurred during migration: " << e.what()
			    << std::endl;
		return false;
	}
	return true;
}

const bool ServerEnvironment::isCompatPlayerModel(const std::string &model_name)
{
	return m_server->isCompatPlayerModel(model_name);
}
