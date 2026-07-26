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

#include <cstdio>
#include <cstdlib>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "s_abm_f.h"
#include "server.h"
#include "log.h"
#include "filesys.h"
#include "porting.h"
#include "common/c_internal.h"
#include "environment.h"
#include "common/c_packer.h"
#include "lua_api/l_base.h"
#include "common/c_converter.h"
#include "common/c_content.h"

ABME::~ABME()
{
	for (ABMT *thread__ : threads) {
		thread__->stop();
	}

	for (auto it : threads) {
		(void)it;
		smp.post();
	}

	// Wait for threads to finish
	actionstream << "ABME-ABMT: Waiting for " << threads.size() << " threads" << std::endl;

	for (ABMT *thread__ : threads) {
		thread__->wait();
	}

	for (ABMT *thread__ : threads) {
		delete thread__;
	}

	abmsMutex.lock();
	abms.clear();
	abmsMutex.unlock();
	threads.clear();
}

void ABME::reg_init_l(StateInitializer func)
{
	FATAL_ERROR_IF(init_done, "Initializer may not be registered after init");
	initializers.push_back(func);
}

void ABME::init()
{
	init_done = true;
	maxthreads = Thread::getNumberOfProcessors() - 1; //dont use main thread
	actionstream << "Using "<<maxthreads<<" threads for ABME" << std::endl;
	addthread();
}

void ABME::addthread()
{
	actionstream << "Placing new thread for ABME [" + itos(threads.size()) + "]" << std::endl;
	ABMT *ta = new ABMT(this, std::string("ABME-") + itos(threads.size()));
	threads.push_back(ta);
	ta->start();
}

bool ABME::QueueABM(std::string &&function, v3s16 p, MapNode n, u32 aoc, u32 aocw)
{
	abmsMutex.lock();
	abms.emplace_back();
	auto &to_add = abms.back();
	to_add.function = std::move(function); //Lua function [Serialized]
	to_add.node = n; //Mapnode
	to_add.pos = p; //Position
	to_add.aoc = aoc; //Active Object Count
	to_add.aocw = aocw; //Active Object Count Wider
	abmsMutex.unlock();
	smp.post();
	return true;
}

void ABME::step() {
	scale();
}

void ABME::scale()
{
	if (threads.size() >= maxthreads)
		return;

	MutexAutoLock autolock(abmsMutex);

	// 2) If the timer elapsed, check again
	if (porting::getTimeMs() >= ast) {
		ast = 0;
		// Determine overlap with previous snapshot
		//unsigned int n = 0;
		//for (const auto &it : abms)
		//	n += asj.count(it.id);
			
		//asj.clear();
		
		//actionstream << "ABME: " << n << " jobs were still waiting after 1s" << std::endl;
		// Start this many new threads
		while (threads.size() < maxthreads) {
			addthread();
		//	n--;
		}
		return;
	}

	// 1) Check if there's anything in the queue
	if (!ast && !abms.empty()) {
		// Take a snapshot of all jobs we have seen
		//for (const auto &it : abms)
		//	asj.emplace(it.id);
		// and set a timer for 1 second
		ast = porting::getTimeMs() + 1000;
	}
}

bool ABME::doenv(lua_State* L, int top)
{
	for (StateInitializer &stateInitializer : initializers) {
		stateInitializer(L, top);
	}

	auto *script = ModApiBase::getScriptApiBase(L);
	try {
		script->loadMod(Server::getBuiltinLuaPath() + "/init.lua", BUILTIN_MOD_NAME);
		//script->checkSetByBuiltin();
	} catch (const ModError &e) {
		errorstream << "Execution of ABME failed!: " << e.what() << std::endl;
		FATAL_ERROR("Executon of ABME has failed");
		return false;
	}

	return true;
}

bool ABME::getavailableabm(ABME_data *job)
{
	smp.wait();
	abmsMutex.lock();
	bool retval = false;
	if (!abms.empty()) {
		*job = std::move(abms.front());
		abms.pop_front();
		retval = true;
	}
	abmsMutex.unlock();

	return retval;
}

ABMT::ABMT(ABME* jobDispatcher, const std::string &name) :
	ScriptApiBase(ScriptingType::Async),
	Thread(name),
	jobDispatcher(jobDispatcher)
{
	lua_State *L = getStack();

	if (jobDispatcher->server) {
		setGameDef(jobDispatcher->server);

		if (g_settings->getBool("secure.enable_security"))
			initializeSecurity();
	}

	// Prepare job lua environment
	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	// Push builtin initialization type
	lua_pushstring(L, jobDispatcher->server ? "async_game" : "async");
	lua_setglobal(L, "INIT");

	if (!jobDispatcher->doenv(L, top)) {
		// can't throw from here so we're stuck with this
		error = true;
	}
	lua_pop(L, 1);
}

ABMT::~ABMT()
{
	sanity_check(!isRunning());
}

void* ABMT::run()
{
	if (error)
		return nullptr;

	lua_State *L = getStack();

	int error_handler = PUSH_ERROR_HANDLER(L);

	auto report_error = [this] (const ModError &e) {
		if (jobDispatcher->server)
			jobDispatcher->server->setAsyncFatalError(e.what());
		else
			errorstream << e.what() << std::endl;
	};

	lua_getglobal(L, "core");
	if (lua_isnil(L, -1)) {
		FATAL_ERROR("Could not find 'core' on ABME [On ABMT] environment!");
	}

	// Main loop
	ABME_data abm;
	
	while (!stopRequested()) {
		// ABM Functions
		//std::cout << "s" << std::endl;
		if (jobDispatcher->getavailableabm(&abm) && !stopRequested()) {
			//Load serialized function
			if (luaL_loadbuffer(L, abm.function.data(), abm.function.size(), "=(ABME)")) {
				errorstream << "On ABMT from ABME: Unable to deserialize a ABM function!" << std::endl;
				lua_pushnil(L);
			}
			
			//ServerEnvironment env = jobDispatcher->server->getEnv()
			push_v3s16(L, abm.pos);
			pushnode(L, abm.node, ModApiBase::ENV->getGameDef()->ndef());
			lua_pushnumber(L, abm.aoc);
			lua_pushnumber(L, abm.aocw);
			int result = lua_pcall(L, 4, 1, error_handler);
			if (result) {
				try {
					scriptError(result, "[ABME]");
				} catch (const ModError &e) {
					report_error(e);
				}
			}
			lua_pop(L, 1);  // Pop retval
		}
	}

	lua_pop(L, 2);  // Pop core and error handler

	return 0;
}

