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

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "abm_threading.h"
#include "serverenvironment.h"
#include "server.h"

/*
	ActiveBlockList
*/

void fillRadiusBlock(v3s16 p0, s16 r, std::set<v3s16> &list)
{
	v3s16 p;
	for(p.X=p0.X-r; p.X<=p0.X+r; p.X++)
		for(p.Y=p0.Y-r; p.Y<=p0.Y+r; p.Y++)
			for(p.Z=p0.Z-r; p.Z<=p0.Z+r; p.Z++)
			{
				// limit to a sphere
				if (p.getDistanceFrom(p0) <= r) {
					// Set in list
					list.insert(p);
				}
			}
}

void fillViewConeBlock(v3s16 p0,
	const s16 r,
	const v3f camera_pos,
	const v3f camera_dir,
	const float camera_fov,
	std::set<v3s16> &list)
{
	v3s16 p;
	const s16 r_nodes = r * BS * MAP_BLOCKSIZE;
	for (p.X = p0.X - r; p.X <= p0.X+r; p.X++)
	for (p.Y = p0.Y - r; p.Y <= p0.Y+r; p.Y++)
	for (p.Z = p0.Z - r; p.Z <= p0.Z+r; p.Z++) {
		if (isBlockInSight(p, camera_pos, camera_dir, camera_fov, r_nodes)) {
			list.insert(p);
		}
	}
}

void ActiveBlockList::update(std::vector<PlayerSAO*> &active_players,
	s16 active_block_range,
	s16 active_object_range,
	std::set<v3s16> &blocks_removed,
	std::set<v3s16> &blocks_added)
{
	/*
		Create the new list
	*/
	std::set<v3s16> newlist = m_forceloaded_list;
	m_abm_list = m_forceloaded_list;
	for (const PlayerSAO *playersao : active_players) {
		v3s16 pos = getNodeBlockPos(floatToInt(playersao->getBasePosition(), BS));
		fillRadiusBlock(pos, active_block_range, m_abm_list);
		fillRadiusBlock(pos, active_block_range, newlist);

		s16 player_ao_range = std::min(active_object_range, playersao->getWantedRange());
		// only do this if this would add blocks
		if (player_ao_range > active_block_range) {
			v3f camera_dir = v3f(0,0,1);
			camera_dir.rotateYZBy(playersao->getLookPitch());
			camera_dir.rotateXZBy(playersao->getRotation().Y);
			fillViewConeBlock(pos,
				player_ao_range,
				playersao->getEyePosition(),
				camera_dir,
				playersao->getFov(),
				newlist);
		}
	}

	/*
		Find out which blocks on the old list are not on the new list
	*/
	// Go through old list
	for (v3s16 p : m_list) {
		// If not on new list, it's been removed
		if (newlist.find(p) == newlist.end())
			blocks_removed.insert(p);
	}

	/*
		Find out which blocks on the new list are not on the old list
	*/
	// Go through new list
	for (v3s16 p : newlist) {
		// If not on old list, it's been added
		if(m_list.find(p) == m_list.end())
			blocks_added.insert(p);
	}

	/*
		Update m_list
	*/
	m_list.clear();
	for (v3s16 p : newlist) {
		m_list.insert(p);
	}
}

ABMWithState::ABMWithState(ActiveBlockModifier *abm_):
	abm(abm_)
{
	// Initialize timer to random value to spread processing
	float itv = abm->getTriggerInterval();
	itv = MYMAX(0.001, itv); // No less than 1ms
	int minval = MYMAX(-0.51*itv, -60); // Clamp to
	int maxval = MYMIN(0.51*itv, 60);   // +-60 seconds
	timer = myrand_range(minval, maxval);
}

//On Close Everything
ABME_Threading::~ABME_Threading()
{
	actionstream << "ABME_Threading: Stopping & killing threads...." << std::endl;
	for (ABMT_Threading *thread__ : threads) {
		thread__->stop();
	}
	//for (auto it : threads) {
	//	(void)it;
	//	smp.post();
	//}
	// Wait for threads to finish
	actionstream << "ABME_Threading: Waiting for " << threads.size() << " threads" << std::endl;
	for (ABMT_Threading *thread__ : threads) {
		thread__->wait();
	}
	for (ABMT_Threading *thread__ : threads) {
		delete thread__;
	}
	ActiveBlocks.clear();
	for (ABMWithState &m_abm : RegisteredABMS) {
		delete m_abm.abm;
	}
}

void ABME_Threading::init()
{
	maxthreads = Thread::getNumberOfProcessors()-1; //dont use main thread
	actionstream << "Using "<<maxthreads<<" threads for ABME_Threading" << std::endl;
	addthread();
	//Mark
	infostream << "Getting user-set configuration..." << std::endl;
	m_cache_abm_interval = g_settings->getFloat("abm_interval");
	infostream << "m_cache_abm_interval=" << m_cache_abm_interval << std::endl;
	m_cache_abm_time_budget = g_settings->getFloat("abm_time_budget");
	infostream << "m_cache_abm_time_budget=" << m_cache_abm_time_budget << std::endl;
	AbmEngineInitialized = true;
}

void ABME_Threading::addthread()
{
	actionstream << "Placing new thread for ABM_Threading [" + itos(threads.size()) + "]" << std::endl;
	ABMT_Threading *ta = new ABMT_Threading(this, std::string("ABM_Threading-") + itos(threads.size()));
	threads.push_back(ta);
	ta->start();
}

ABMT_Threading::~ABMT_Threading()
{
	infostream << "Thread down" << std::endl;
}

bool ABME_Threading::QueueMapBlock(std::vector<std::vector<ActiveABM>*> handler, MapBlock *block)
{
	ABM_MapBlocksMUTEX.lock();
	ABM_MapBlocks.emplace_back();
	auto &to_add = ABM_MapBlocks.back();
	to_add.block = block;
	to_add.m_aabms = handler;
	ABM_MapBlocksMUTEX.unlock();
	return true;
}

void ABME_Threading::startengine()
{
	infostream << "ABME_Threading::startengine() called Threads(handled, max)[" << threads.size() << ", " << maxthreads << "]" << std::endl;
	ScaleWasRun = true;
	MutexAutoLock autolock(ABM_MapBlocksMUTEX);
	while (threads.size() < maxthreads) {
		addthread();
	}
	infostream << "ABME_Threading::startengine(): Total threads: " << threads.size() << std::endl;
}

bool ABME_Threading::get_queued_mapblock_to_load(ABME_Mapblock *job)
{
	ABM_MapBlocksMUTEX.lock();
	bool retval = false;
	if (!ABM_MapBlocks.empty()) {
		*job = std::move(ABM_MapBlocks.front());
		ABM_MapBlocks.pop_front();
		retval = true;
	}
	ABM_MapBlocksMUTEX.unlock();
	return retval;
}

void ABME_Threading::set_server_started() {
	server_started = true;
}

void ABME_Threading::registerabm(ActiveBlockModifier *abm) {
	infostream << "Registering abm..." << std::endl;
	RegisteredABMS.emplace_back(abm);
}

void ABME_Threading::delete_block(v3s16 pos) {
	ActiveBlocks.m_list.erase(pos);
}

void ABME_Threading::delete_block_abmlist(v3s16 pos) {
	ActiveBlocks.m_abm_list.erase(pos);
}

ReturnValues ABME_Threading::update_list_n_return(std::vector<ABMWithState> ABMS, float dtime_s, bool use_timers) {
	const NodeDefManager *ndef = m_env->getGameDef()->ndef();
	//create table
	std::vector<std::vector<ActiveABM>*> m_aabms;
	for (ABMWithState &ABMwS : ABMS) {
		//get abm
		ActiveBlockModifier *abm = ABMwS.abm;
		
		//get interval
		float trigger_interval = abm->getTriggerInterval();
		if(trigger_interval < 0.001)
			trigger_interval = 0.001;
			
		//copy dtime_s
		float actual_interval = dtime_s;
		
		//timer
		if (use_timers) {
			ABMwS.timer += dtime_s;
			if(ABMwS.timer < trigger_interval)
				continue;
			ABMwS.timer -= trigger_interval;
			actual_interval = trigger_interval;
		}
		
		//chance
		float chance = abm->getTriggerChance();
		if(chance == 0)
			chance = 1;
			
		//chance & interval
		ActiveABM aabm;
		aabm.abm = abm;
		if (abm->getSimpleCatchUp()) {
			if (actual_interval < trigger_interval)
				continue;
			float intervals = actual_interval / trigger_interval;
			
			aabm.chance = chance / intervals;
			if(aabm.chance == 0)
				aabm.chance = 1;
		} else {
			aabm.chance = chance;
		}
		//limits
		aabm.min_y = abm->getMinY();
		aabm.max_y = abm->getMaxY();
		
		//neighbors
		const std::vector<std::string> &required_neighbors_s = abm->getRequiredNeighbors();
		for (const std::string &required_neighbor_s : required_neighbors_s) {
			ndef->getIds(required_neighbor_s, aabm.required_neighbors);
		}
		aabm.check_required_neighbors = !required_neighbors_s.empty();
		
		//contents
		const std::vector<std::string> &contents_s = abm->getTriggerContents();
		
		//process
		for (const std::string &content_s : contents_s) {
			std::vector<content_t> ids;
			ndef->getIds(content_s, ids);
			for (content_t c : ids) {
				if (c >= m_aabms.size()) {
					//m_aabms.resize(c + 256, NULL);
					m_aabms.reserve(c + 256);
					m_aabms.resize(c + 256);
				}
				if (!m_aabms[c])
					m_aabms[c] = new std::vector<ActiveABM>;
				m_aabms[c]->push_back(aabm);
			}
		}
	}
	ReturnValues abmglb = {ABMS, m_aabms}; //LIST OF MODIFIED ABMS, LIST OF ABMS TO BE EFFECT
	return abmglb;
}

//Thread Safe cache on Mapblocks
//void ABME_Threading::Queue

std::vector<std::vector<ActiveABM>*> ABME_Threading::trigger_actions(std::vector<std::vector<ActiveABM>*> m_aabms, MapBlock *block) {
	if (block->isDummy()) {
		return m_aabms;
	}
	
	u32 active_object_count_wider;
	u32 active_object_count = countObjects(block, active_object_count_wider);
	m_env->m_added_objects = 0;
	v3s16 p0;
	std::vector<std::tuple<ActiveBlockModifier*, v3s16, MapNode, u32, u32>> trigger_queue;
	for(p0.X=0; p0.X<MAP_BLOCKSIZE; p0.X++)
		for(p0.Y=0; p0.Y<MAP_BLOCKSIZE; p0.Y++)
			for(p0.Z=0; p0.Z<MAP_BLOCKSIZE; p0.Z++) {
				const MapNode &n = block->getNodeUnsafe(p0);
				content_t c = n.getContent();
				// Cache content types as we go
				if (!block->contents_cached && !block->do_not_cache_contents) {
					block->contents.insert(c);
					if (block->contents.size() > 64) {
						// Too many different nodes... don't try to cache
						block->do_not_cache_contents = true;
						block->contents.clear();
					}
				}
				if (c >= m_aabms.size() || !m_aabms[c])
					continue;
				v3s16 p = p0 + block->getPosRelative();
				for (ActiveABM &aabm : *m_aabms[c]) {
					if ((p.Y < aabm.min_y) || (p.Y > aabm.max_y))
						continue;
					if (myrand() % aabm.chance != 0)
						continue;
					// Check neighbors
					if (aabm.check_required_neighbors) {
						v3s16 p1;
						for(p1.X = p0.X-1; p1.X <= p0.X+1; p1.X++)
						for(p1.Y = p0.Y-1; p1.Y <= p0.Y+1; p1.Y++)
						for(p1.Z = p0.Z-1; p1.Z <= p0.Z+1; p1.Z++)
						{
							if(p1 == p0)
								continue;
							content_t c;
							if (block->isValidPosition(p1)) {
								// if the neighbor is found on the same map block
								// get it straight from there
								const MapNode &n = block->getNodeUnsafe(p1);
								c = n.getContent();
							} else {
								// otherwise consult the map
								MapNode n = m_map->getNode(p1 + block->getPosRelative());
								c = n.getContent();
							}
							if (CONTAINS(aabm.required_neighbors, c))
								goto neighbor_found;
						}
						// No required neighbor found
						continue;
					}
					neighbor_found:
					trigger_queue.emplace_back(aabm.abm, p, n, active_object_count, active_object_count_wider);
					// Count surrounding objects again if the abms added any
					if(m_env->m_added_objects > 0) {
						active_object_count = countObjects(block, active_object_count_wider);
						m_env->m_added_objects = 0;
					}
				}
			}
		//_
	//_
	for (auto &t : trigger_queue) {
		queueTriggerForLua(std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t), std::get<4>(t));
	}
	block->contents_cached = !block->do_not_cache_contents;
	return m_aabms;
}

u32 ABME_Threading::countObjects(MapBlock *block, u32 &wider) {
	wider = 0;
	u32 wider_unknown_count = 0;
	for(s16 x=-1; x<=1; x++)
		for(s16 y=-1; y<=1; y++)
			for(s16 z=-1; z<=1; z++)
			{
				MapBlock *block2 = m_map->getBlockNoCreateNoEx(
					block->getPos() + v3s16(x,y,z));
				if(block2==NULL){
					wider_unknown_count++;
					continue;
				}
				wider += block2->m_static_objects.m_active.size()
					+ block2->m_static_objects.m_stored.size();
			}
	// Extrapolate
	u32 active_object_count = block->m_static_objects.m_active.size();
	u32 wider_known_count = 3*3*3 - wider_unknown_count;
	wider += wider_unknown_count * wider / wider_known_count;
	return active_object_count;
}

/*
	QUEUER FOR ENVIRONMENT IN MAIN LUA STATE
	Tchaikovsy the GOAT
	
	queueTriggerForLua() will queue functions of abm triggering, because it can not load in the same thread or else it will crash
*/
void ABME_Threading::queueTriggerForLua(ActiveBlockModifier *abm, v3s16 pos, MapNode node, u32 aoc, u32 aocw) {
	//ServerEnvironment will be used, heh heh heh
	m_env->QueueTriggerForABM(abm, pos, node, aoc, aocw);
}

// Checks if queue is full
bool ABME_Threading::isQueuedABMsAtCapacity(size_t &bv) {
	ABM_MapBlocksMUTEX.lock();
	size_t size = ABM_MapBlocks.size();
	ABM_MapBlocksMUTEX.unlock();
	bv = size;
	if (size >= 512)
		return true;
	else
		return false;
}

size_t ABME_Threading::GetSizeOfQueue() {
	ABM_MapBlocksMUTEX.lock();
	size_t s = ABM_MapBlocks.size();
	ABM_MapBlocksMUTEX.unlock();
	return s;
}

void ABME_Threading::step() {
	//Might need to check
	{
		//Should we check abms
		size_t abmslist = GetSizeOfQueue();
		if (abmslist == 0)
			EmptyList = true;
		else {
			EmptyList = false;
			return; // Return.
		}
	}
	
	if (EmptyList) {
		//Update list
		std::vector<ABMWithState> list = RegisteredABMS;
		ReturnValues RETURNEDBALLUTT = update_list_n_return(list, m_cache_abm_interval, true);
		
		//Query list
		std::vector<std::vector<ActiveABM>*> m_aabms = RETURNEDBALLUTT.m_aabms;
		std::vector<ABMWithState> abmlistupdated = RETURNEDBALLUTT.ABMS;
		
		//Save list
		RegisteredABMS = abmlistupdated;
		
		//Shuffle list & query for mapblocks
		std::vector<v3s16> output(ActiveBlocks.m_abm_list.size());
		std::copy(ActiveBlocks.m_abm_list.begin(), ActiveBlocks.m_abm_list.end(), output.begin());
		std::shuffle(output.begin(), output.end(), m_rgen);
		
		//Queue to load in worker threads
		bool ran_contents = false;
		for (const v3s16 &p : output) {
			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			
			if (m_aabms.empty()) {
				continue;
			}
			
			if (!block) {
				errorstream << "No available mapblock! (SIGSEGV blocked!)" << std::endl;
				continue;
			}
			
			// Check the content type cache first
			// to see whether there are any ABMs
			// to be run at all for this block.
			if (block->contents_cached) {
				bool run_abms = false;
				for (content_t c : block->contents) {
					if (c < m_aabms.size() && m_aabms[c]) {
						run_abms = true;
						break;
					}
				}
				if (!run_abms)
					continue;
			} else {
				// Clear any caching
				block->contents.clear();
			}
			ran_contents = true;
			QueueMapBlock(m_aabms, block);
		}
		
		//Wake up threads
		if (ran_contents) {
			smp.post();
			infostream << "Waking up threads" << std::endl;
		}
	}
}
void* ABMT_Threading::run()
{
	actionstream << "*ABMT_Threading" << std::endl;
	//GET CONFIG
	if (!g_settings->getBool("do_abm")) {
		return nullptr;
	}
	
	//LOOP
	ABME_Mapblock ABMErawDATA;
	while (!stopRequested()) {
		if (!jobDispatcher->get_queued_mapblock_to_load(&ABMErawDATA) || stopRequested()) {
			jobDispatcher->smp.wait();
			continue;
		}
		TimeTaker timer("modify in active blocks per interval");
		std::vector<std::vector<ActiveABM>*> m_aabms = ABMErawDATA.m_aabms;
		MapBlock *block = ABMErawDATA.block;
		
		if (!block) {
			continue;
		}
		
		if (m_aabms.empty()) {
			timer.stop(true);
			continue;
		}
		
		block->setTimestampNoChangedFlag(jobDispatcher->m_env->m_game_time);
		
		u32 max_time_ms = jobDispatcher->m_cache_abm_interval * 1000 * jobDispatcher->m_cache_abm_time_budget;
		jobDispatcher->trigger_actions(m_aabms, block);
		u32 time_ms = timer.getTimerTime();
		
		if (time_ms > max_time_ms) {
			warningstream << "active block modifiers took " << time_ms << "ms Thread[" << std::this_thread::get_id() << "]" << std::endl;
		}
		
		timer.stop(true);
	}
	return 0;
}




















