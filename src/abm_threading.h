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

/*
	ACTIVE BLOCK MODIFIER THREADING
	[0] IF (__E) DO CRASH(); END;
	[1] IF (STATE == NOT_GOOD) DO RETURN(); END;
	[2] 
	[3] 
	[4] unarrivedfactor::0
	-------------------------------
	
	
*/

#pragma once

#include "serverenvironment.h"

#include <vector>
#include <deque>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <set>
#include <random>

#include "threading/semaphore.h"
#include "threading/thread.h"
#include "util/timetaker.h"
#include "environment.h"
#include "server/player_sao.h"
#include "mapnode.h"
#include "server.h"
#include "mapblock.h"
#include "map.h"

//class ServerEnvironment;

class ActiveBlockModifier
{
public:
	ActiveBlockModifier() = default;
	virtual ~ActiveBlockModifier() = default;

	// Set of contents to trigger on
	virtual const std::vector<std::string> &getTriggerContents() const = 0;
	// Set of required neighbors (trigger doesn't happen if none are found)
	// Empty = do not check neighbors
	virtual const std::vector<std::string> &getRequiredNeighbors() const = 0;
	// Trigger interval in seconds
	virtual float getTriggerInterval() = 0;
	// Random chance of (1 / return value), 0 is disallowed
	virtual u32 getTriggerChance() = 0;
	// Whether to modify chance to simulate time lost by an unnattended block
	virtual bool getSimpleCatchUp() = 0;
	// get min Y for apply abm
	virtual s16 getMinY() = 0;
	// get max Y for apply abm
	virtual s16 getMaxY() = 0;
	// This is called usually at interval for 1/chance of the nodes
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n){};
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
		u32 active_object_count, u32 active_object_count_wider){};
};

struct ActiveABM
{
	ActiveBlockModifier *abm;
	int chance;
	std::vector<content_t> required_neighbors;
	bool check_required_neighbors; // false if required_neighbors is known to be empty
	s16 min_y;
	s16 max_y;
};

struct ABMWithState
{
	ActiveBlockModifier *abm;
	float timer = 0.0f;

	ABMWithState(ActiveBlockModifier *abm_);
};

struct ReturnValues {
	std::vector<ABMWithState> ABMS;
	std::vector<std::vector<ActiveABM>*> m_aabms;
};
/*
class ABMHandler {
public:
	//updates the abm list
	ABMHandler(std::vector<ABMWithState> &abms, float dtime_s, ServerEnvironment *env, bool use_timers):
		m_env(env)
	{
		
	}

	
	This repeats every cicle, which the user defines, default is 0.2
	THIS updates the list of active abms
	
	THIS MODIFIES THE REGISTERED ABM TO BE MODIFIED
	
	

	

	~ABMHandler()
	{
		for (auto &aabms : m_aabms)
			delete aabms;
	}

	// Find out how many objects the given block and its neighbours contain.
	// Returns the number of objects in the block, and also in 'wider' the
	// number of objects in the block and all its neighbours. The latter
	// may an estimate if any neighbours are unloaded.
	
	void apply(MapBlock *block, int &blocks_scanned, int &abms_run, int &blocks_cached)
	{
		
	}
};*/

class ActiveBlockList
{
public:
	void update(std::vector<PlayerSAO*> &active_players,
		s16 active_block_range,
		s16 active_object_range,
		std::set<v3s16> &blocks_removed,
		std::set<v3s16> &blocks_added);

	bool contains(v3s16 p){
		return (m_list.find(p) != m_list.end());
	}

	void clear(){
		m_list.clear();
	}

	std::set<v3s16> m_list;
	std::set<v3s16> m_abm_list;
	std::set<v3s16> m_forceloaded_list;
};

class ABME_Threading;

/*
	THIS SCRIPT ONLY DO ABM ON MAPBLOCKS AND RUN THE ABM TO SEND INTO ABM THREADS
*/

//
// Should be used for .apply method on different threads to handle data, also using JobDispatcher.
// apply(MapBlock *block, int &blocks_scanned, int &abms_run, int &blocks_cached)
// ^^^^^
// When completely executed, should return scanned blocks, cached blocks and running abms
// Optimized should be only scanned blocks, other are only for debugs
//

//CLEAR: 2177, 2190, 308::serverenvironment.h

//proccess queue of mapblocks which have been added from active block list
struct ABME_Mapblock
{
	ABME_Mapblock() = default;
	MapBlock *block;
	std::vector<std::vector<ActiveABM>*> m_aabms;
};

class ABMT_Threading : public Thread {
	friend class ABME_Threading;
public:
	virtual ~ABMT_Threading();
	void *run();
protected:
	ABMT_Threading(ABME_Threading* jobDispatcher, const std::string &name) : Thread(name), jobDispatcher(jobDispatcher) {};
private:
	ABME_Threading *jobDispatcher = nullptr;
};

class ABME_Threading {
	friend class ABMT_Threading;
public:
	ABME_Threading() = default;
	ABME_Threading(ServerEnvironment *environment_, ServerMap *_map) : m_env(environment_), m_map(_map) {};
	~ABME_Threading();
	void init(); //select all threads
	//void update_list(); //update mapblocks list
	void set_server_started();
	void delete_block(v3s16 pos);
	bool QueueMapBlock(std::vector<std::vector<ActiveABM>*> handler, MapBlock *block); //queue MapBlock to load
	void delete_block_abmlist(v3s16 pos);
	void registerabm(ActiveBlockModifier *abm);
	ActiveBlockList ActiveBlocks; //Stored mapblocks & abms
	bool server_started = false;
	void startengine();
	bool already_scaled = false;
	
	std::vector<std::vector<ActiveABM>*> trigger_actions(std::vector<std::vector<ActiveABM>*> m_aabms, MapBlock *block);
	ReturnValues update_list_n_return(std::vector<ABMWithState> ABMS, float dtime_s, bool use_timers);
	u32 countObjects(MapBlock *block, u32 &wider);
	
	void queueTriggerForLua(ActiveBlockModifier *abm, v3s16 pos, MapNode node, u32 aoc, u32 aocw);
	//void queueTriggerForLua(ActiveBlockModifier *abm, v3s16 pos, MapNode node);
	
	//To be exec in server environment (abme->step())
	void step();
	
	bool isQueuedABMsAtCapacity(size_t &bv);
	size_t GetSizeOfQueue();
protected:
	void addthread();
	bool get_queued_mapblock_to_load(ABME_Mapblock *job);
private:
	u64 ast = 0;
	bool init_done = false;
	unsigned int maxthreads = 0;
	
	std::mutex ABM_MapBlocksMUTEX;
	std::deque<ABME_Mapblock> ABM_MapBlocks; //.apply
	
	std::vector<ABMWithState> RegisteredABMS; // Active Blocks, with state
	//std::deque<ABMWithState> ActiveBlocksState;
	
	std::vector<ABMT_Threading*> threads; //Threads
	
	ServerEnvironment *m_env; //Shared environment
	ServerMap *m_map; //Shared map
	
	std::mt19937 m_rgen; //for random
	
	std::thread::id main_threadID; //Main thread id
	
	float m_cache_abm_interval;
	u32 m_game_time;
	float m_cache_abm_time_budget;
	
	bool AbmEngineInitialized = false;
	bool is_main_thread_getted = false;
	bool GotWorkerThread = false;
	bool ScaleWasRun = false;
	bool WarningShown = false;
	bool WorkingThreads = false;
	bool EmptyList = true;
	bool MainThreadWaitUntilQueueIsEmpty = false;
	
	Semaphore smp;
};


































