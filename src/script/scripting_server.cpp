/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "scripting_server.h"
#include "server.h"
#include "log.h"
#include "settings.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_areastore.h"
#include "lua_api/l_auth.h"
#include "lua_api/l_base.h"
#include "lua_api/l_craft.h"
#include "lua_api/l_env.h"
#include "lua_api/l_inventory.h"
#include "lua_api/l_item.h"
#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_mapgen.h"
#include "lua_api/l_modchannels.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_nodetimer.h"
#include "lua_api/l_noise.h"
#include "lua_api/l_object.h"
#include "lua_api/l_playermeta.h"
#include "lua_api/l_particles.h"
#include "lua_api/l_rollback.h"
#include "lua_api/l_server.h"
#include "lua_api/l_util.h"
#include "lua_api/l_vmanip.h"
#include "lua_api/l_settings.h"
#include "lua_api/l_http.h"
#include "lua_api/l_storage.h"
#include "lua_api/l_abm.h"

extern "C" {
//#include <lua.h>
#include <lualib.h>
}

ServerScripting::ServerScripting(Server* server):
		ScriptApiBase(ScriptingType::Server),
		asyncEngine(server),
		abme(server)
{
//	std::cout << "dbg: -SRT" << std::endl;
	setGameDef(server);

	// setEnv(env) is called by ScriptApiEnv::initializeEnvironment()
	// once the environment has been created

//	std::cout << "dbg: SRT" << std::endl;

	SCRIPTAPI_PRECHECKHEADER

//	std::cout << "dbg: SRT_" << std::endl;

	if (g_settings->getBool("secure.enable_security")) {
		initializeSecurity();
	} else {
		warningstream << "\\!/ Mod security should never be disabled, as it allows any mod to "
				<< "access the host machine."
				<< "Mods should use minetest.request_insecure_environment() instead \\!/" << std::endl;
	}
	
	ModApiBase::CCL = L;
	
//std::cout << "dbg: SRT_1" << std::endl;
	lua_getglobal(L, "core");
	int top = lua_gettop(L);
//std::cout << "dbg: SRT_2" << std::endl;
	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");
//std::cout << "dbg: SRT_3" << std::endl;
	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");
//std::cout << "dbg: SRT_4" << std::endl;
	// Initialize our lua_api modules
	InitializeModApi(L, top);
	lua_pop(L, 1);
//std::cout << "dbg: SRT_5" << std::endl;
	// Push builtin initialization type
	lua_pushstring(L, "game");
	lua_setglobal(L, "INIT");
//std::cout << "dbg: SRT_6" << std::endl;
	infostream << "SCRIPTAPI: Initialized game modules" << std::endl;
}

void ServerScripting::initAsync()
{
	// Save globals to transfer
	/*
	{
		lua_State *L = getStack();
		lua_getglobal(L, "core");
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "get_globals_to_transfer");
		lua_call(L, 0, 1);
		luaL_checktype(L, -1, LUA_TSTRING);
		getServer()->m_async_globals_data.set(readParam<std::string>(L, -1));
		lua_pushnil(L);
		lua_setfield(L, -3, "get_globals_to_transfer"); // unset function too
		lua_pop(L, 2); // pop 'core', return value
	}*/

	 //std::cout << "dbg: 1" << std::endl;

	actionstream << "SCRIPTAPI: Initializing async engine" << std::endl;
	asyncEngine.registerStateInitializer(InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiUtil::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiCraft::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiItemMod::InitializeAsync);
	asyncEngine.registerStateInitializer(ModApiServer::InitializeAsync);
	// not added: ModApiMapgen is a minefield for thread safety
	// not added: ModApiHttp async api can't really work together with our jobs
	// not added: ModApiStorage is probably not thread safe(?)
//std::cout << "dbg: 2" << std::endl;
	asyncEngine.initialize(0);
//	std::cout << "dbg: 3" << std::endl;

	actionstream << "SCRIPTAPI: Initializing abme" << std::endl;
	abme.reg_init_l(InitializeAsync);
	abme.reg_init_l(ModApiUtil::InitializeAsync);
	abme.reg_init_l(ModApiCraft::InitializeAsync);
	abme.reg_init_l(ModApiItemMod::InitializeAsync);
	abme.reg_init_l(ModApiServer::InitializeAsync);
	abme.reg_init_l(ModApiAbmT::Initialize);
	abme.init();
}

void ServerScripting::stepAsync()
{
	asyncEngine.step(getStack());
}

void ServerScripting::dostepabm() {
	abme.step();
}

u32 ServerScripting::queueAsync(std::string &&serialized_func,
	PackedValue *param, const std::string &mod_origin)
{
	return asyncEngine.queueAsyncJob(std::move(serialized_func),
			param, mod_origin);
}

void ServerScripting::queueABM(std::string &&func, v3s16 p, MapNode n, u32 aoc, u32 aocw) {
	abme.QueueABM(std::move(func), p, n, aoc, aocw);
}

void ServerScripting::saveGlobals()
{
	SCRIPTAPI_PRECHECKHEADER
//	std::cout << "dbg: 1CR" << std::endl;
	lua_getglobal(L, "core");
//	std::cout << "dbg: 1CR2" << std::endl;
	luaL_checktype(L, -1, LUA_TTABLE);
//	std::cout << "dbg: 1CR3" << std::endl;
	lua_getfield(L, -1, "get_globals_to_transfer");
//std::cout << "dbg: 1CR4" << std::endl;
	lua_call(L, 0, 1);
//std::cout << "dbg: 1CR5" << std::endl;
	auto *data = script_pack(L, -1);
//std::cout << "dbg: 1CR6" << std::endl;
	assert(!data->contains_userdata);
//std::cout << "dbg: 1CR7" << std::endl;
	getServer()->m_lua_globals_data.reset(data);
//std::cout << "dbg: 1CR8" << std::endl;
	// unset the function
//std::cout << "dbg: 1CR9" << std::endl;
	lua_pushnil(L);
//std::cout << "dbg: 1CR10" << std::endl;
	lua_setfield(L, -3, "get_globals_to_transfer");
//std::cout << "dbg: 1CR11" << std::endl;
	lua_pop(L, 2); // pop 'core', return value
}

void ServerScripting::InitializeAsync(lua_State *L, int top)
{
	// classes
	LuaItemStack::Register(L);
	LuaPerlinNoise::Register(L);
	LuaPerlinNoiseMap::Register(L);
	LuaPseudoRandom::Register(L);
	LuaPcgRandom::Register(L);
	LuaSecureRandom::Register(L);
	LuaVoxelManip::Register(L);
	LuaSettings::Register(L);
	NodeTimerRef::Register(L);

	// globals data
	/*lua_getglobal(L, "core");
	luaL_checktype(L, -1, LUA_TTABLE);
	std::string s = ModApiBase::getServer(L)->m_async_globals_data.get();
	lua_pushlstring(L, s.c_str(), s.size());
	lua_setfield(L, -2, "transferred_globals");
	lua_pop(L, 1); // pop 'core'*/
	// globals data
	
//	std::cout << "dbg: 1CR_1" << std::endl;
	auto *data = ModApiBase::getServer(L)->m_lua_globals_data.get();
//	std::cout << "dbg: 1CR_2" << std::endl;
	assert(data);
//	std::cout << "dbg: 1CR_3" << std::endl;
	script_unpack(L, data);
//	std::cout << "dbg: 1CR_4" << std::endl;
	lua_setfield(L, top, "transferred_globals");
}

void ServerScripting::InitializeModApi(lua_State *L, int top)
{
	// Register reference classes (userdata)
	InvRef::Register(L);
	ItemStackMetaRef::Register(L);
	LuaAreaStore::Register(L);
	LuaItemStack::Register(L);
	LuaPerlinNoise::Register(L);
	LuaPerlinNoiseMap::Register(L);
	LuaPseudoRandom::Register(L);
	LuaPcgRandom::Register(L);
	LuaRaycast::Register(L);
	LuaSecureRandom::Register(L);
	LuaVoxelManip::Register(L);
	NodeMetaRef::Register(L);
	NodeTimerRef::Register(L);
	ObjectRef::Register(L);
	PlayerMetaRef::Register(L);
	LuaSettings::Register(L);
	StorageRef::Register(L);
	ModChannelRef::Register(L);

	// Initialize mod api modules
	ModApiAuth::Initialize(L, top);
	ModApiCraft::Initialize(L, top);
	ModApiEnvMod::Initialize(L, top);
	ModApiInventory::Initialize(L, top);
	ModApiItemMod::Initialize(L, top);
	ModApiMapgen::Initialize(L, top);
	ModApiParticles::Initialize(L, top);
	ModApiServer::Initialize(L, top);
	ModApiUtil::Initialize(L, top);
	ModApiHttp::Initialize(L, top);
	ModApiStorage::Initialize(L, top);
	ModApiChannels::Initialize(L, top);
}
