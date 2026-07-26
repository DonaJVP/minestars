/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, Donatto J. Viveros. P. <donatto555@gmail.com>
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

#include "../server.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include "../filesys.h"
#include "../log.h"
#include "../util/serialize.h"
#include "../map.h"
#include "../emerge.h"
//#include "lib/mt_map.h"
#include "../threading/thread.h"
#include "../irr_v3d.h"
#include "../remoteplayer.h"
#include "../mapblock.h"
#include "../ServerNetworkEngine.h"
#include "../slave_helpers.h"
#include "map.h"
#include <atomic>

#define MAPS_DATA_VERSION "MineStars::MapsData::v1.00"
#define MAPS_DATA_VERSION_LEN 27

//NOTE: huh? will this work?
//NOTE: i love raw binary files!

//NOTE: Auto saver

//useful functions, not gonna use some generated ones
static uint16_t read16(std::istream &is) {
	char buf[2] = {0};
	is.read(buf, sizeof(buf));
	return readU16((uint8_t*)buf);
}

//BEGIN AutoSaver

void *ServerMapFilesSaver::run() {
	while (!stopRequested()) {
		clock0 += 100;
		if (clock0 > 1000) { //Save player positions
			saveAllPlayersPositions();
			clock0 = 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); //0.1ms
	}
}

void ServerMapFilesSaver::init() {
	Players = &m_server->PlayerToMap;
	m_path_world = m_server->m_path_world;
};

void ServerMapFilesSaver::saveAllPlayersPositions() {
	std::ostringstream os(std::ios_base::binary);
	char buff[64];
	char msdh[MAPS_DATA_VERSION_LEN] = MAPS_DATA_VERSION;
	uint16_t pcount = Players->getSize();
	os.write(msdh, MAPS_DATA_VERSION_LEN); //Write header
	writeU16(os, pcount); //Player count
	Players->Lock();
	for (auto it = Players->GetRawMap()->begin(); it != Players->GetRawMap()->end(); it++) {
		std::string pname = it->first;
		//Compile
		writeU16(os, it->second.mapid);
		//First, name len and name(str)
		writeU16(os, pname.size());
		strcpy(buff, pname.c_str());
		os.write(buff, pname.size());
		//Now like-a-table
		writeU16(os, it->second.MAP.size());
		for (auto it_ = it->second.MAP.begin(); it_ != it->second.MAP.end(); it_++) {
			//Always save last positions too, if the server crashes, the server saves his positions and this too.
			//[MAPID][v3f]
			writeU16(os, (uint16_t)it_->first); //MAP_ID
			writeV3F(os, it_->second, 42); //Use the most precise value
		}
	}
	Players->unLock();
	fs::safeWriteToFile(m_server->m_path_world+"/players_map_data.msd", os.str());
	actionstream << FUNCTION_NAME << ": Map player save files complete!" << std::endl;
}

void ServerMapFilesSaver::loadFiles() {
	std::string rawdata;
	bool loadedsuccessfully = fs::ReadFile(m_path_world+"/players_map_data.msd", rawdata);
	if (!loadedsuccessfully) {
		warningstream << FUNCTION_NAME << ": Could not load players_map_data.msd.. Using defaults" << std::endl;
		return;
	}
	std::istringstream is(rawdata, std::ios_base::binary);
	//Here it starts.....
	char msdh[MAPS_DATA_VERSION_LEN];
	is.read(msdh, sizeof(msdh));
	char msdh_[MAPS_DATA_VERSION_LEN] = MAPS_DATA_VERSION;
	if (strcmp(msdh, msdh_) != 0) {
		errorstream << FUNCTION_NAME << ": Unknown version of MapData{MineStarsData}: " << msdh << std::endl;
		return;
	}
	//Read the players count
	uint16_t count = read16(is);
	for (uint16_t i = 0; i < count; i++) {
		//Get player name and his size
		uint16_t mapid_ = read16(is);
		uint16_t pname_size_ = read16(is);
		char buff[64]; //Only allow 64 chars for the playername
		is.read(buff, pname_size_);
		std::string name;
		name.assign(buff);
		// Let's see the elements on the table, it are sans-tastic (Asgore i'm sorry....)
		uint16_t saved_map_count_ = read16(is);
		std::unordered_map<MAP_ID, v3f> umap;
		PlayerDataOMM pdata;
		pdata.mapid = (MAP_ID)mapid_; //Assign main map
		for (uint16_t smc = 0; smc < saved_map_count_; smc++) {
			//[Mapid][V3f]
			//Try to save directly on the files
			uint16_t stored_map_id_ = read16(is);
			v3f coords = readV3F32(is);
			umap[stored_map_id_] = coords;
		}
		pdata.MAP = umap;
		Players->Set(name, pdata);
	}
}

//END AutoSaver

//BEGIN MapThread

#define SLEEPMS_FORMAPTHREAD 100

class MapThread: public Thread, public MapEventReceiver {
public:
	MapThread(Server *serv, std::string thr_name, ServerMap *map, EmergeManager *emerge): Thread(thr_name), m_server(serv), m_map(map), m_emerge(emerge) {}
	void *run() {
		int_fast16_t clock0 = 0, clock1 = 0;
		while (!stopRequested()) {
			clock0 += 1;
			clock1 += 1;
			//Wake up threads
			if (clock0 > 59) { //0.2s
				m_emerge->startThreads();
				clock0 = 0;
			}
			if (clock1 > 2000) { //5.5s
				m_map->save(MOD_STATE_WRITE_NEEDED);
				clock1 = 0;
			}
			// Well, now is about send and sent
			{
				bool disable_single_change_sending = false;
				if(m_unsent_map_edit_queue.size() >= 4)
					disable_single_change_sending = true;
				std::list<v3s16> node_meta_updates;
				while (!m_unsent_map_edit_queue.empty()) {
					MapEditEvent *event = m_unsent_map_edit_queue.pop();
					if (event == nullptr) //FIXME: Should fix this
						continue;
					std::unordered_set<u16> far_players;
					switch (event->type) {
						case MEET_ADDNODE:
						case MEET_SWAPNODE:
							m_server->sendAddNode(event->p, event->n, &far_players, disable_single_change_sending ? 5 : 30, event->type == MEET_ADDNODE);
							break;
						case MEET_REMOVENODE:
							m_server->sendRemoveNode(event->p, &far_players, disable_single_change_sending ? 5 : 30);
							break;
						case MEET_BLOCK_NODE_METADATA_CHANGED: {
							if (!event->is_private_change) {
								// Don't send the change yet. Collect them to eliminate dupes.
								node_meta_updates.remove(event->p);
								node_meta_updates.push_back(event->p);
							}
							if (MapBlock *block = m_map->getBlockNoCreateNoEx(getNodeBlockPos(event->p))) {
								block->raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_REPORT_META_CHANGE);
							}
							break;
						}
						case MEET_OTHER:
							if (!m_server->ServersNetworkObject->AreSlave) {
								m_server->m_clients.lock();
								for (const v3s16 &modified_block : event->modified_blocks) {
									m_server->m_clients.markBlockposAsNotSent(modified_block);
								}
								m_server->m_clients.unlock();
							}
							break;
						default:
							break;
					}
					if (!far_players.empty()) {
						// Convert list format to that wanted by SetBlocksNotSent
						std::map<v3s16, MapBlock*> modified_blocks2;
						for (const v3s16 &modified_block: event->modified_blocks) {
							modified_blocks2[modified_block] = m_map->getBlockNoCreateNoEx(modified_block);
						}
						// Set blocks not sent LLK
						if (!m_server->ServersNetworkObject->AreSlave) {
							for (const u16 far_player: far_players) {
								try {
									RemoteClient *client = m_server->getClient(far_player);
									m_server->SetBlocksNotSent(modified_blocks2);
								} catch (std::exception &e) {
								}
							}
						} else {
							for (const u16 far_player: far_players) {
								if (!m_server->ClientDataTable.Has(far_player))
									continue;
								ClientDataHelper *client = m_server->ClientDataTable.Get(far_player);
								if (client)
									client->SetBlocksNotSent(modified_blocks2);
							}
						}
					}
					delete event;
				}
				if (node_meta_updates.size() > 0)
					m_server->sendMetadataChanged(node_meta_updates);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	void onMapEditEvent(const MapEditEvent &event) {
		// Here stands every modification done to submaps.
		m_unsent_map_edit_queue.push(new MapEditEvent(event));
	};
private:
	Server *m_server = nullptr;
	ServerMap *m_map = nullptr;
	EmergeManager *m_emerge = nullptr;
	MultithreadQueue<MapEditEvent*> m_unsent_map_edit_queue;
	//MapUpdate *mu = nullptr;
};

//END MapThread

//BEGIN MapDataManipulator

/*
 * Map Files Structure:
 * 1. 26 bytes <Header>: MineStars::MapsData::v1.0
 * 2. 2 Bytes: Maps count
 * 3. [MapId::Uint16][MapNameLenght::Uint16][MapName]
 * 4. ....
 * 5. Players count
 * 6. [MapId::Uint16][PlayerNameLenght::Uint16][PlayerName]
 * 7. ...
 */

//Saves all info about the maps & players standing in
void Server::saveMapFiles() {
	std::ostringstream raw(std::ios_base::binary);
	char buff[64]; //64 characters long can be the name
	char msdh[MAPS_DATA_VERSION_LEN] = MAPS_DATA_VERSION; //Null terminator includes
	raw.write(msdh, sizeof(msdh));
	//Do it.
	//Write Maps count
	uint16_t maps = Maps.size();
	writeU16(raw, maps);
	//Write Maps
	for (auto it = Maps.begin(); it != Maps.end(); it++) {
		writeU16(raw, it->first);
		writeU16(raw, (uint16_t)it->second->m_name.size());
		strcpy(buff, it->second->m_name.c_str());
		raw.write(buff, it->second->m_name.size());
	}
	//Write players environment
	/*PlayerToMap.Lock();
	writeU16(raw, PlayerToMap.GetRawMap().size());
	for (const auto it = PlayerToMap.GetRawMap().begin(); it != PlayerToMap.GetRawMap().end(); it++) {
		//Start saving
		writeU16(raw, it->second);
		writeU16(raw, (uint16_t)it->first.size());
		strcpy(buff, it->first.c_str());
		raw.write(buff, it->first.size());

	}
	PlayerToMap.UnLock();*/
	fs::safeWriteToFile(m_path_world+"/maps.msd", raw.str());
	actionstream << FUNCTION_NAME << ": Map save files complete!" << std::endl;
}

//Loads players current positions
void Server::loadMapFiles() {
	std::string rawdata;
	bool loadedsuccessfully = fs::ReadFile(m_path_world+"/maps.msd", rawdata);
	std::istringstream iss(rawdata, std::ios_base::binary);
	if (!loadedsuccessfully) {
		warningstream << FUNCTION_NAME << ": Seems the server is loading for the first time, using default values for map engine" << std::endl;
		return;
	}
	//Load the header bytes
	char msdh[MAPS_DATA_VERSION_LEN];
	iss.read(msdh, sizeof(msdh));
	char msdh_[MAPS_DATA_VERSION_LEN] = MAPS_DATA_VERSION;
	if (strcmp(msdh, msdh_) != 0) {
		errorstream << FUNCTION_NAME << ": Unknown version of MapData{MineStarsData}: " << msdh << std::endl;
		return;
	}
	//Parse info to system
	//Load the maps
	uint16_t maps_c = read16(iss);
	for (uint16_t i = 0; i < maps_c; i++) {
		//Read maps data
		uint16_t mapid = read16(iss);
		uint16_t mapnamelenght = read16(iss);
		//Map name will occupy 64byte buffer
		char buff[64];
		iss.read(buff, mapnamelenght);
		//Register map
		HyperMap *map = new HyperMap();
		map->m_id = mapid;
		std::string name;
		name.append(buff, mapnamelenght);
		map->m_name = name;
		//Initialize map & emerge (NOTE: Should emerge be in a different environment?, FIXME: yep)
		std::string path;
		path.append(m_path_world);
		path.append("/");
		path.append(name);
		fs::CreateAllDirs(m_path_world+"/"+name);
		EmergeManager *emerge_m = new EmergeManager(this, map);
		ServerMap *smap = new ServerMap(path, this, emerge_m, m_metrics_backend.get());
		//Save map info
		map->m_emerge = emerge_m;
		map->m_map = smap;
		Maps[mapid] = map;

		//Initialize the map thread
		MapThread *m_thr = new MapThread(this, "Map::"+name, smap, emerge_m);
		map->m_thread = m_thr;
		m_thr->start();

		actionstream << FUNCTION_NAME << ": Registered map: " << name << "::" << path << "::" << mapid << std::endl;
	}

	//Load the players
	/*uint16_t players = read16(iss);
	for (uint16_t i = 0; i < players; i++) {
		uint16_t mapid = read16(iss);
		uint16_t namelen = read16(iss);
		char name[namelen]; //idk the real limit so i will do in the fantastic way
		iss.read(name, namelen); // Please don't make a segmentation fault
		std::string standart;
		standart.append(name, namelen);
		//Register
		PlayerToMap.Set(standart, mapid);
		actionstream << FUNCTION_NAME << ": Registered player <" << standart << "> on map <" << Maps.at(mapid)->m_name << std::endl;
	}*/

	actionstream << FUNCTION_NAME << ": Registering done!" << std::endl;

	actionstream << FUNCTION_NAME << ": Initializing AutoSaver for maps" << std::endl;

	m_smfs = new ServerMapFilesSaver(this);
	m_smfs->init();
}

//END MapDataManipulator

//BEGIN MapManagement

bool Server::createNewMap(const std::string mapname, uint16_t *mapid) {
	for (auto it = Maps.begin(); it != Maps.end(); it++) {
		if (it->second->m_name == mapname) {
			warningstream <<FUNCTION_NAME << ": Already registered map!: " << mapname << std::endl;
			*mapid = 0;
			return false;
		}
	}
	//Map doens't exist, so create a new one
	HyperMap *map = new HyperMap();
	map->m_name = mapname;
	map->m_id = Maps.size()+1; //I should use vector?
	//Here the magic starts
	std::string path;
	path.append(m_path_world);
	path.append("/");
	path.append(mapname);
	fs::CreateAllDirs(path);
	EmergeManager *emerge_m = new EmergeManager(this, map);
	ServerMap *smap = new ServerMap(path, this, emerge_m, m_metrics_backend.get());
	if (!smap->settings_mgr.makeMapgenParams())
		FATAL_ERROR("Couldn't create any mapgen type");
	emerge_m->initMapgens(smap->getMapgenParams());
	map->m_map = smap;
	map->m_emerge = emerge_m;
	actionstream << FUNCTION_NAME << ": Registered map: " << mapname << "::" << path << "::" << (Maps.size()+1) << std::endl;
	*mapid = Maps.size()+1;
	Maps[Maps.size()+1] = map;

	//Initialize the map thread
	MapThread *m_thr = new MapThread(this, "Map::"+mapname, smap, emerge_m);
	map->m_thread = m_thr;
	// Make mapeditevents notify each player.
	smap->addEventReceiver(m_thr);
	m_thr->start();

	//Save to database
	saveMapFiles();

	return true;
}

bool Server::deleteMap(uint16_t mapid) {
	if (Maps.find(mapid) == Maps.end()) {
		warningstream << FUNCTION_NAME << ": Unknown map: " << mapid << std::endl;
		return false;
	}

	//Try to move players to the main world
	MapToPlayers.Lock();
	for (auto it = MapToPlayers.Get(mapid)->GetRawMap()->begin(); it != MapToPlayers.Get(mapid)->GetRawMap()->end(); it++) {
		 // [[BIG SHOT]]
		setPlayerOnMap(it->second, 0, PlayerToMap.Get(it->second->getName()).MAP.at(0));
	}
	MapToPlayers.unLock();

	// [[BIG SHOT]]

	HyperMap *map = Maps.at(mapid);
	delete map->m_map;
	delete map->m_emerge;
	map->m_thread->stop();
	delete map->m_thread;
	Maps.erase(mapid);

	//Save to database
	saveMapFiles();
	return true;
}


//END MapManagement

































