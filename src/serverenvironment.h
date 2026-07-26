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

#pragma once

#include "activeobject.h"
#include "environment.h"
#include "mapnode.h"
#include "server.h"
#include "settings.h"
#include "server/activeobjectmgr.h"
#include "util/numeric.h"
#include "constants.h"
#include "core/lib/mt_queue.h"
#include "server/player_sao.h"
//#include "abm_threading.h"
#include <algorithm>
#include <cstdint>
#include <set>
#include <random>

class IGameDef;
class ServerMap;
struct GameParams;
class MapBlock;
class RemotePlayer;
class PlayerDatabase;
class AuthDatabase;
class PlayerSAO;
class ServerEnvironment;
class ActiveBlockModifier;
struct StaticObject;
class ServerActiveObject;
class Server;
class ServerScripting;
class ABME_Threading;
class ActiveBlockModifier;
class NetworkPacket;

//Nodes to apply
struct PerNodeQueue {
	PerNodeQueue() = default;
	v3s16 pos;
	MapNode node;
	uint8_t type;
	uint16_t mapid;
	bool from_abm = false;
};

struct TriggeringLuaQueue {
	TriggeringLuaQueue() = default;
	ActiveBlockModifier *abm;
	v3s16 pos;
	MapNode node;
	u32 aoc; // Active Object Count
	u32 aocw; // Active Object Count Wider
};

struct _M_M_OPP {
	_M_M_OPP() = default;
	PlayerSAO *sao = nullptr;
	ServerActiveObject *puncher = nullptr;
	float time_from_last_punch;
	ToolCapabilities *toolcap;
	v3f dir;
	s16 hp;
};
struct _M_M_OR {
	_M_M_OR() = default;
	PlayerSAO *s = nullptr;
	ServerActiveObject *c = nullptr;
};
struct _M_M_OHC {
	_M_M_OHC() = default;
	PlayerSAO *sao = nullptr;
	PlayerHPChangeReason PHCR;
	u16 HP;
};

struct LoadingBlockModifierDef
{
	// Set of contents to trigger on
	std::set<std::string> trigger_contents;
	std::string name;
	bool run_at_every_load = false;

	virtual ~LoadingBlockModifierDef() = default;

	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n){};
};

struct LBMContentMapping
{
	typedef std::unordered_map<content_t, std::vector<LoadingBlockModifierDef *>> lbm_map;
	lbm_map map;

	std::vector<LoadingBlockModifierDef *> lbm_list;

	// Needs to be separate method (not inside destructor),
	// because the LBMContentMapping may be copied and destructed
	// many times during operation in the lbm_lookup_map.
	void deleteContents();
	void addLBM(LoadingBlockModifierDef *lbm_def, IGameDef *gamedef);
	const std::vector<LoadingBlockModifierDef *> *lookup(content_t c) const;
};

class LBMManager
{
public:
	LBMManager() = default;
	~LBMManager();

	// Don't call this after loadIntroductionTimes() ran.
	void addLBMDef(LoadingBlockModifierDef *lbm_def);

	void loadIntroductionTimes(const std::string &times,
		IGameDef *gamedef, u32 now);

	// Don't call this before loadIntroductionTimes() ran.
	std::string createIntroductionTimesString();

	// Don't call this before loadIntroductionTimes() ran.
	void applyLBMs(ServerEnvironment *env, MapBlock *block, u32 stamp);

	// Warning: do not make this std::unordered_map, order is relevant here
	typedef std::map<u32, LBMContentMapping> lbm_lookup_map;

private:
	// Once we set this to true, we can only query,
	// not modify
	bool m_query_mode = false;

	// For m_query_mode == false:
	// The key of the map is the LBM def's name.
	// TODO make this std::unordered_map
	std::map<std::string, LoadingBlockModifierDef *> m_lbm_defs;

	// For m_query_mode == true:
	// The key of the map is the LBM def's first introduction time.
	lbm_lookup_map m_lbm_lookup;

	// Returns an iterator to the LBMs that were introduced
	// after the given time. This is guaranteed to return
	// valid values for everything
	lbm_lookup_map::const_iterator getLBMsIntroducedAfter(u32 time)
	{ return m_lbm_lookup.lower_bound(time); }
};

/*
	Operation mode for ServerEnvironment::clearObjects()
*/
enum ClearObjectsMode {
	// Load and go through every mapblock, clearing objects
		CLEAR_OBJECTS_MODE_FULL,

	// Clear objects immediately in loaded mapblocks;
	// clear objects in unloaded mapblocks only when the mapblocks are next activated.
		CLEAR_OBJECTS_MODE_QUICK,
};

struct HyperMap;

class ServerEnvironment : public Environment
{
public:
	ServerEnvironment(ServerMap *map, ServerScripting *scriptIface, Server *server, const std::string &path_world, std::unordered_map<uint16_t,
HyperMap*> *maps);
	~ServerEnvironment();

	Map & getMap(uint16_t mapid = 0);

	ServerMap & getServerMap(uint16_t mapid = 0);

	//TODO find way to remove this fct!
	ServerScripting* getScriptIface()
	{ return m_script; }

	Server *getGameDef()
	{ return m_server; }

	float getSendRecommendedInterval()
	{ return m_recommended_send_interval; }

	void kickAllPlayers(AccessDeniedCode reason,
		const std::string &str_reason, bool reconnect);
	// Save players
	void saveLoadedPlayers(bool force = false);
	void savePlayer(RemotePlayer *player);
	PlayerSAO *loadPlayer(RemotePlayer *player, bool *new_player, uint16_t peer_id, uint16_t mapid = 0);
	void addPlayer(RemotePlayer *player);
	void removePlayer(RemotePlayer *player);
	bool removePlayerFromDatabase(const std::string &name);

	/*
		Save and load time of day and game timer
	*/
	void saveMeta();
	void loadMeta();

	u32 addParticleSpawner(float exptime);
	u32 addParticleSpawner(float exptime, u16 attached_id);
	void deleteParticleSpawner(u32 id, bool remove_from_object = true);

	/*
		External ActiveObject interface
		-------------------------------------------
	*/

	ServerActiveObject* getActiveObject(u16 id)
	{
		return m_ao_manager.getActiveObject(id);
	}

	/*
		Add an active object to the environment.
		Environment handles deletion of object.
		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	u16 addActiveObject(ServerActiveObject *object, uint16_t mapid = 0);

	/*
		Add an active object as a static object to the corresponding
		MapBlock.
		Caller allocates memory, ServerEnvironment frees memory.
		Return value: true if succeeded, false if failed.
		(note:  not used, pending removal from engine)
	*/
	//bool addActiveObjectAsStatic(ServerActiveObject *object);

	/*
		Find out what new objects have been added to
		inside a radius around a position
	*/
	void getAddedActiveObjects(PlayerSAO *playersao, s16 radius,
		s16 player_radius,
		std::set<u16> &current_objects,
		std::queue<u16> &added_objects);

	/*
		Find out what new objects have been removed from
		inside a radius around a position
	*/
	void getRemovedActiveObjects(PlayerSAO *playersao, s16 radius,
		s16 player_radius,
		std::set<u16> &current_objects,
		std::queue<u16> &removed_objects);

	u32 m_game_time = 0;

	/*
		Get the next message emitted by some active object.
		Returns false if no messages are available, true otherwise.
	*/
	bool getActiveObjectMessage(ActiveObjectMessage *dest);

	virtual void getSelectedActiveObjects(
		const core::line3d<f32> &shootline_on_map,
		std::vector<PointedThing> &objects
	);

	//Queuing tru____
	void QueueTriggerForABM(ActiveBlockModifier *abm, v3s16 pos, MapNode node, u32 aoc, u32 aocw);
	void TriggeringLuaOnStep();

	/*
		Activate objects and dynamically modify for the dtime determined
		from timestamp and additional_dtime
	*/
	void activateBlock(MapBlock *block, u32 additional_dtime=0, uint16_t mapid = 0);

	/*
		{Active,Loading}BlockModifiers
		-------------------------------------------
	*/

	void addActiveBlockModifier(ActiveBlockModifier *abm);
	void addLoadingBlockModifierDef(LoadingBlockModifierDef *lbm);

	/*
		Other stuff
		-------------------------------------------
	*/

	// Script-aware node setters
	bool setNode(v3s16 p, const MapNode &n, uint16_t mapid = 0);
	bool removeNode(v3s16 p, uint16_t mapid = 0);
	bool swapNode(v3s16 p, const MapNode &n, uint16_t mapid = 0);

	void QueueNodeModify(int mode, v3s16 pos, const MapNode &node, bool byabm, uint16_t mapid = 0);
	void QueuedNodesStep();

	// Find the daylight value at pos with a Depth First Search
	u8 findSunlight(v3s16 pos, uint16_t mapid) const;

	// Find all active objects inside a radius around a point
	void getObjectsInsideRadius(std::vector<ServerActiveObject *> &objects, const v3f &pos, float radius,
			std::function<bool(ServerActiveObject *obj)> include_obj_cb, uint16_t mapid = 0)
	{
		return m_ao_manager.getObjectsInsideRadius(pos, radius, objects, include_obj_cb, mapid);
	}

	// Find all active objects inside a box
	void getObjectsInArea(std::vector<ServerActiveObject *> &objects, const aabb3f &box,
			std::function<bool(ServerActiveObject *obj)> include_obj_cb, uint16_t mapid = 0)
	{
		return m_ao_manager.getObjectsInArea(box, objects, include_obj_cb, mapid);
	}

	// Clear objects, loading and going through every MapBlock
	void clearObjects(ClearObjectsMode mode, uint16_t mapid);

	// This makes stuff happen
	void step(f32 dtime);
	void stepScript(float dtime);

	u32 getGameTime() const { return m_game_time; }

	void reportMaxLagEstimate(float f) { m_max_lag_estimate = f; }
	float getMaxLagEstimate() { return m_max_lag_estimate; }

	std::set<v3s16>* getForceloadedBlocks();

	// Sets the static object status all the active objects in the specified block
	// This is only really needed for deleting blocks from the map
	void setStaticForActiveObjectsInBlock(v3s16 blockpos,
		bool static_exists, v3s16 static_block=v3s16(0,0,0));

	RemotePlayer *getPlayer(const session_t peer_id);
	RemotePlayer *getPlayerByID(const uint16_t id);
	RemotePlayer *getPlayer(const char* name);
	const std::vector<RemotePlayer *> getPlayers(); //const { return m_players; }
	u32 getPlayerCount(); //const { return m_players.size(); }

	static bool migratePlayersDatabase(const GameParams &game_params,
			const Settings &cmd_args);

	AuthDatabase *getAuthDatabase() { return m_auth_database; }
	static bool migrateAuthDatabase(const GameParams &game_params,
			const Settings &cmd_args);

	const bool isCompatPlayerModel(const std::string &model_name);
	inline bool getCompatSendOriginalModel() { return m_compat_send_original_model; }

	/**
	 * called if env_meta.txt doesn't exist (e.g. new world)
	 */
	void loadDefaultMeta();

	bool getWorldSpawnpoint(v3f &spawnpoint) {
		if (m_has_world_spawnpoint)
			spawnpoint = m_world_spawnpoint;
		return m_has_world_spawnpoint;
	}

	void setWorldSpawnpoint(const v3f &spawnpoint) {
		m_world_spawnpoint = spawnpoint;
		m_has_world_spawnpoint = true;
	}

	void resetWorldSpawnpoint() {
		m_has_world_spawnpoint = false;
	}
	
	//Variable to lock functions from being used on other again!
	bool doingModifyToMap = false;

	RemotePlayer *FindPlayerWithThisId(u16 id);
	void stepPlayers();
	void queueOnChangeHP(PlayerSAO *player, u16 hp, const PlayerHPChangeReason reason);
	void queueOnRightClickPlayer(PlayerSAO *p, ServerActiveObject *sao);
	void queuePlayerEvent(PlayerSAO *s);
	void queueOnPunchPlayer(PlayerSAO *playersao, ServerActiveObject *puncher, float time_from_last_punch, const ToolCapabilities *toolcap, const v3f dir, u16 hp);
	// Server definition
	Server *m_server;
private:
	MultithreadQueue<PlayerSAO*> m_m_player_queue_queuePlayerEvent;
	MultithreadQueue<_M_M_OPP> m_m_player_queue_on_punchplayer;
	MultithreadQueue<_M_M_OR> m_m_player_queue_on_rightclickplayer;
	MultithreadQueue<_M_M_OHC> m_m_player_queue_on_hpchange;
	std::unordered_map<std::string, uint16_t> PnameToID;
	std::unordered_map<u16, u16> PlayerMAPwithOBJID;
	std::unordered_map<u16, u16> DirectionsMAP;
	static PlayerDatabase *openPlayerDatabase(const std::string &name,
			const std::string &savedir, const Settings &conf);
	static AuthDatabase *openAuthDatabase(const std::string &name,
			const std::string &savedir, const Settings &conf);
	/*
		Internal ActiveObject interface
		-------------------------------------------
	*/

	/*
		Add an active object to the environment.

		Called by addActiveObject.

		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	u16 addActiveObjectRaw(ServerActiveObject *object, bool set_changed, u32 dtime_s);
	u16 addActiveObjectRaw(ServerActiveObject *object, bool set_changed, u32 dtime_s, uint16_t mapid = 0);

	/*
		Remove all objects that satisfy (isGone() && m_known_by_count==0)
	*/
	void removeRemovedObjects();

	/*
		Convert stored objects from block to active
	*/
	void activateObjects(MapBlock *block, u32 dtime_s, uint16_t mapid = 0);

	/*
		Convert objects that are not in active blocks to static.

		If m_known_by_count != 0, active object is not deleted, but static
		data is still updated.

		If force_delete is set, active object is deleted nevertheless. It
		shall only be set so in the destructor of the environment.
	*/
	void deactivateFarObjects(bool force_delete);

	/*
		A few helpers used by the three above methods
	*/
	void deleteStaticFromBlock(
			ServerActiveObject *obj, u16 id, u32 mod_reason, bool no_emerge);
	bool saveStaticToBlock(v3s16 blockpos, u16 store_id,
			ServerActiveObject *obj, const StaticObject &s_obj, u32 mod_reason);

	/*
		Member variables
	*/

	// The map
	ServerMap *m_map;
	// Lua state
	ServerScripting* m_script;
	// Active Object Manager
	server::ActiveObjectMgr m_ao_manager; //Main
	// World path
	const std::string m_path_world;
	// Outgoing network message buffer for active objects
	std::queue<ActiveObjectMessage> m_active_object_messages;
	// Some timers
	float m_send_recommended_timer = 0.0f;
	IntervalLimiter m_object_management_interval;
	// List of active blocks
	//ActiveBlockList m_active_blocks;
	IntervalLimiter m_active_blocks_management_interval;
	IntervalLimiter m_active_block_modifier_interval;
	IntervalLimiter m_active_blocks_nodemetadata_interval;
	// Whether the variables below have been read from file yet
	bool m_meta_loaded = false;
	// Time from the beginning of the game in seconds.
	// A helper variable for incrementing the latter
	float m_game_time_fraction_counter = 0.0f;
	// Time of last clearObjects call (game time).
	// When a mapblock older than this is loaded, its objects are cleared.
	u32 m_last_clear_objects_time = 0;
	// Active block modifiers
	//std::vector<ABMWithState> m_abms;
	LBMManager m_lbm_mgr;
	// An interval for generally sending object positions and stuff
	float m_recommended_send_interval = 0.1f;
	// Estimate for general maximum lag as determined by server.
	// Can raise to high values like 15s with eg. map generation mods.
	float m_max_lag_estimate = 0.1f;

	// peer_ids in here should be unique, except that there may be many 0s
	std::vector<RemotePlayer*> m_players;

	PlayerDatabase *m_player_database = nullptr;
	AuthDatabase *m_auth_database = nullptr;

	// Pseudo random generator for shuffling, etc.
	std::mt19937 m_rgen;

	// Particles
	IntervalLimiter m_particle_management_interval;
	std::unordered_map<u32, float> m_particle_spawners;
	std::unordered_map<u32, u16> m_particle_spawner_attachments;

	std::vector<std::string> m_compat_player_models;
	bool m_compat_send_original_model;

	v3f m_world_spawnpoint = v3f(0.f, 0.f, 0.f);
	bool m_has_world_spawnpoint = false;

	ServerActiveObject* createSAO(ActiveObjectType type, v3f pos, const std::string &data,
uint16_t mapid);
	
	std::unordered_map<u16, RemotePlayer*> StoredPlayersIDs;
	
	//debug
#ifdef NDEBUG
	bool debugging = false;
#else
	bool debugging = true;
#endif
	//queues
		//Nodes to apply
		std::mutex QueueMutex;
		std::deque<PerNodeQueue> nodestoapply;
	
		//Lua ABM functions to apply
		std::mutex QueueTriggerMutex;
		std::deque<TriggeringLuaQueue> triggeringluatoapply;
	
	std::unordered_map<uint16_t, HyperMap*> *Maps = nullptr;

	//ticks
	float serverenvtimer = 0.0f; //DEFAULT, CAN BE MODIFIED
	float m_abm_timer = 0.0f;
	
	//Active Block Modifier Engine+Threading
	ABME_Threading *abme;
	
	
};
