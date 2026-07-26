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

#include "cpp_api/s_env.h"
#include "cpp_api/s_base.h"
#include "cpp_api/s_internal.h"
#include "common/c_converter.h"
#include "log.h"
#include "environment.h"
#include "mapgen/mapgen.h"
#include "lua_api/l_env.h"
#include "server.h"
#include "player.h"
#include "server/player_sao.h"
#include "lua_api/l_object.h"
#include "server/luaentity_sao.h"

void ScriptApiEnv::environment_OnGenerated(v3s16 minp, v3s16 maxp,
	u32 blockseed)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_generateds
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_generateds");
	// Call callbacks
	push_v3s16(L, minp);
	push_v3s16(L, maxp);
	lua_pushnumber(L, blockseed);
	runCallbacks(3, RUN_CALLBACKS_MODE_FIRST);
}
/*
void ScriptApiEnv::environment_Step(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER
	//infostream << "scriptapi_environment_step" << std::endl;

	// Get core.registered_globalsteps
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getServer()->setAsyncFatalError(
				std::string("environment_Step: ") + e.what() + "\n"
				+ script_get_backtrace(L));
	}
}*/

void ScriptApiEnv::environment_Step(float dtime) {
	SCRIPTAPI_PRECHECKHEADER
	
	int error_handler = PUSH_ERROR_HANDLER(L);
	
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "func_on_steps");
	lua_remove(L, -2);
	
	//int error_handler = PUSH_ERROR_HANDLER(L);
	
	if (lua_isfunction(L, -1)) {
		ServerEnvironment *env = (ServerEnvironment *) getEnv();
		//Push neccesary elements
		//Dtime
		lua_pushnumber(L, dtime);
		//Players [list of object refs]
		lua_createtable(L, env->getPlayerCount(), 0);
		u32 i = 0;
		for (RemotePlayer *player : env->getPlayers()) {
			if (player->getPeerId() == PEER_ID_INEXISTENT)
				continue;
			PlayerSAO *sao = player->getPlayerSAO();
			if (sao && !sao->isGone()) {
				objectrefGetOrCreate(L, sao);
				lua_rawseti(L, -2, ++i);
			}
		}
		//Players & Control [Catars = {}, Magnus = {}]
		lua_createtable(L, 0, env->getPlayerCount());
		for (RemotePlayer *player : env->getPlayers()) {
			PlayerSAO *sao = player->getPlayerSAO();
			if (sao && !sao->isGone()) {
				const PlayerControl &c = player->getPlayerControl();
				auto set = [L] (const char *name, bool value) {
					lua_pushboolean(L, value);
					lua_setfield(L, -2, name);
				};
				lua_createtable(L, 0, 13);
				set("up", c.up);
				set("down", c.down);
				set("left", c.left);
				set("right", c.right);
				set("jump", c.jump);
				set("aux1", c.aux1);
				set("sneak", c.sneak);
				set("zoom", c.zoom);
				set("dig", c.dig);
				set("place", c.place);
				objectrefGetOrCreate(L, sao);
				lua_setfield(L, -2, "player");
				
				lua_setfield(L, -2, player->getName());
			}
		}
		
		//Do function
		int result = lua_pcall(L, 3, 0, error_handler);
		if (result)
			scriptError(result, "environment_Step::[core.func_on_steps]");
		
		lua_pop(L, 1);
	} else {
		getServer()->setAsyncFatalError("function [core.func_on_steps(dtime, players, player_control)] not found");
	}
}

void ScriptApiEnv::player_event(ServerActiveObject *player, const std::string &type)
{
	SCRIPTAPI_PRECHECKHEADER

	if (player == NULL)
		return;

	// Get minetest.registered_playerevents
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_playerevents");

	// Call callbacks
	objectrefGetOrCreate(L, player);   // player
	lua_pushstring(L,type.c_str()); // event type
	try {
		runCallbacks(2, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getServer()->setAsyncFatalError(
				std::string("player_event: ") + e.what() + "\n"
				+ script_get_backtrace(L) );
	}
}

void ScriptApiEnv::PLAYER_on_click(RemotePlayer *p, bool dig, bool place) {
	SCRIPTAPI_PRECHECKHEADER
	
	if (!p) {
		errorstream << "Not known player [ScriptApiEnv::PLAYER_on_click]" << std::endl;
		return;
	}
	
	int error_handler = PUSH_ERROR_HANDLER(L);
	
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "func_on_click");
	
	if (lua_isfunction(L, -1)) {
		lua_remove(L, -2);
		objectrefGetOrCreate(L, p->getPlayerSAO());
		lua_pushboolean(L, dig);
		lua_pushboolean(L, place);
		int result = lua_pcall(L, 3, 0, error_handler);
		if (result)
			scriptError(result, "PLAYER_on_click::[core.func_on_click]");
	}
}

void ScriptApiEnv::PLAYER_on_move(RemotePlayer *player, u32 kp, f32 pitch) {
	SCRIPTAPI_PRECHECKHEADER
	
	if (player == NULL) {
		errorstream << "Player is null! [ScriptApiEnv::PLAYER_on_move]" << std::endl;
		return;
	}
	
	int error_handler = PUSH_ERROR_HANDLER(L);
	
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "func_animate_player");
	
	if (lua_isfunction(L, -1)) {
		lua_remove(L, -2);
		//get values
		bool click = (kp & (0x1 << 7)) || (kp & (0x1 << 8));
		bool walk = (kp & (0x1 << 0)) || (kp & (0x1 << 1)) || (kp & (0x1 << 2)) || (kp & (0x1 << 3));
		bool aux = (kp & (0x1 << 5));
		bool sneak = (kp & (0x1 << 6));
		//Check if the pattern is repeated, if so then don't exec
		if (
			player->pt_w == walk &&
			player->pt_a == aux &&
			player->pt_s == sneak
		) {
			//Return if the pattern is the same, function should not call it self.
			return;
		} else {
			player->pt_w = walk;
			player->pt_a = aux;
			player->pt_s = sneak;
		}
		
		if (sneak) {
			player->SsumZ = 2.35f;
			player->SsumY = -1.20f;
		} else {
			player->SsumZ = 0.0f;
			player->SsumY = 0.0f;
		}
		
		//int error_handler = PUSH_ERROR_HANDLER(L);
		
		//process
		objectrefGetOrCreate(L, player->getPlayerSAO());
		lua_pushboolean(L, walk);
		lua_pushboolean(L, aux);
		lua_pushboolean(L, sneak);
		lua_pushboolean(L, click);
		lua_pushinteger(L, pitch);
		//call act
		int result = lua_pcall(L, 6, 0, error_handler);
		if (result)
			scriptError(result, "PLAYER_on_move::[core.func_animate_player]");
	} else {
		getServer()->setAsyncFatalError("function [core.func_animate_player(...)] not found\nYou might be running MineStars-MultiCraft on a non-MineStars environment.");
	}
}

void ScriptApiEnv::refresh_head(RemotePlayer *player, f32 pitch) {
	SCRIPTAPI_PRECHECKHEADER
	
	if (player == nullptr) {
		errorstream << "Player is null! [ScriptApiEnv::PLAYER_on_move]" << std::endl;
		return;
	}
	lua_getglobal(L, "OnChangeSkin");
	if (lua_isfunction(L, -1)) {
		objectrefGetOrCreate(L, player->getPlayerSAO());
		lua_pushinteger(L, pitch);
		int result = lua_pcall(L, 2, 0, 0);
		if (result != 0)
			getServer()->setAsyncFatalError("!refresh_head::[OnChangeSkin]: " + script_get_backtrace(L));
	}
}

void ScriptApiEnv::PLAYER_head_move(RemotePlayer *player, f32 pitch) {
	SCRIPTAPI_PRECHECKHEADER
	
	if (player == nullptr) {
		errorstream << "Player is null! [ScriptApiEnv::PLAYER_on_move]" << std::endl;
		return;
	}
	
	lua_getglobal(L, "MineStars");
	
	if (lua_istable(L, -1)) {
		// MineStars.HeadsObjects[pname]
		lua_getfield(L, -1, "HeadsObjects");
		lua_remove(L, -2);
		
		lua_getfield(L, -1, player->getName());
		//lua_remove(L, -2);
		
		if (lua_isnil(L, -1)) {
			warningstream << "Player may are not registered <On join> <Ignored>" << std::endl;
			return;
		}
		
		ObjectRef *ref = ObjectRef::checkobject(L, -1);
		
		if (ref == nullptr) {
			errorstream << "Object Ref is nil! May will create new..." << std::endl;
			refresh_head(player, pitch);
			return;
		}
		
		ServerActiveObject *sao = ObjectRef::getobject(ref);
		
		if (sao == nullptr) {
			errorstream << "Object Ref is nil! May will create new..." << std::endl;
			refresh_head(player, pitch);
			return;
		}
		
		//process
		sao->setBonePosition(std::string("Head"), v3f(0.0f, 6.65f + player->sumY + player->SsumY, player->sumZ + player->SsumZ), v3f(-pitch, -184, 0.0f));
	} else {
		infostream << "PLAYER_head_move() called on non-MineStars environment" << std::endl;
	}
}

void ScriptApiEnv::initializeEnvironment(ServerEnvironment *env)
{
	SCRIPTAPI_PRECHECKHEADER
	verbosestream << "ScriptApiEnv: Environment initialized" << std::endl;
	setEnv(env);

	/*
		Add {Loading,Active}BlockModifiers to environment
	*/

	//Save variable
	ModApiBase::ENV = env;

	// Get core.registered_abms
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_abms");
	int registered_abms = lua_gettop(L);

	if (!lua_istable(L, registered_abms)) {
		lua_pop(L, 1);
		throw LuaError("core.registered_abms was not a lua table, as expected.");
	}
	lua_pushnil(L);
	while (lua_next(L, registered_abms)) {
		// key at index -2 and value at index -1
		int id = lua_tonumber(L, -2);
		int current_abm = lua_gettop(L);

		std::vector<std::string> trigger_contents;
		lua_getfield(L, current_abm, "nodenames");
		if (lua_istable(L, -1)) {
			int table = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, table)) {
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				trigger_contents.emplace_back(readParam<std::string>(L, -1));
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if (lua_isstring(L, -1)) {
			trigger_contents.emplace_back(readParam<std::string>(L, -1));
		}
		lua_pop(L, 1);

		std::vector<std::string> required_neighbors;
		lua_getfield(L, current_abm, "neighbors");
		if (lua_istable(L, -1)) {
			int table = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, table)) {
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				required_neighbors.emplace_back(readParam<std::string>(L, -1));
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if (lua_isstring(L, -1)) {
			required_neighbors.emplace_back(readParam<std::string>(L, -1));
		}
		lua_pop(L, 1);

		float trigger_interval = 10.0;
		getfloatfield(L, current_abm, "interval", trigger_interval);

		int trigger_chance = 50;
		getintfield(L, current_abm, "chance", trigger_chance);

		bool simple_catch_up = true;
		getboolfield(L, current_abm, "catch_up", simple_catch_up);
		
		s16 min_y = INT16_MIN;
		getintfield(L, current_abm, "min_y", min_y);
		
		s16 max_y = INT16_MAX;
		getintfield(L, current_abm, "max_y", max_y);

		lua_getfield(L, current_abm, "action");
		luaL_checktype(L, current_abm + 1, LUA_TFUNCTION);
		lua_pop(L, 1);

		LuaABM *abm = new LuaABM(L, id, trigger_contents, required_neighbors,
			trigger_interval, trigger_chance, simple_catch_up, min_y, max_y);

		env->addActiveBlockModifier(abm);

		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// Get core.registered_lbms
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_lbms");
	int registered_lbms = lua_gettop(L);

	if (!lua_istable(L, registered_lbms)) {
		lua_pop(L, 1);
		throw LuaError("core.registered_lbms was not a lua table, as expected.");
	}

	lua_pushnil(L);
	while (lua_next(L, registered_lbms)) {
		// key at index -2 and value at index -1
		int id = lua_tonumber(L, -2);
		int current_lbm = lua_gettop(L);

		std::set<std::string> trigger_contents;
		lua_getfield(L, current_lbm, "nodenames");
		if (lua_istable(L, -1)) {
			int table = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, table)) {
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				trigger_contents.insert(readParam<std::string>(L, -1));
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if (lua_isstring(L, -1)) {
			trigger_contents.insert(readParam<std::string>(L, -1));
		}
		lua_pop(L, 1);

		std::string name;
		getstringfield(L, current_lbm, "name", name);

		bool run_at_every_load = getboolfield_default(L, current_lbm,
			"run_at_every_load", false);

		lua_getfield(L, current_lbm, "action");
		luaL_checktype(L, current_lbm + 1, LUA_TFUNCTION);
		lua_pop(L, 1);

		LuaLBM *lbm = new LuaLBM(L, id, trigger_contents, name,
			run_at_every_load);

		env->addLoadingBlockModifierDef(lbm);

		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

void ScriptApiEnv::on_emerge_area_completion(
	v3s16 blockpos, int action, ScriptCallbackState *state)
{
	Server *server = getServer();

	// This function should be executed with envlock held.
	// The caller (LuaEmergeAreaCallback in src/script/lua_api/l_env.cpp)
	// should have obtained the lock.
	// Note that the order of these locks is important!  Envlock must *ALWAYS*
	// be acquired before attempting to acquire scriptlock, or else ServerThread
	// will try to acquire scriptlock after it already owns envlock, thus
	// deadlocking EmergeThread and ServerThread

	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, state->callback_ref);
	luaL_checktype(L, -1, LUA_TFUNCTION);

	push_v3s16(L, blockpos);
	lua_pushinteger(L, action);
	lua_pushinteger(L, state->refcount);
	lua_rawgeti(L, LUA_REGISTRYINDEX, state->args_ref);

	setOriginDirect(state->origin.c_str());

	try {
		PCALL_RES(lua_pcall(L, 4, 0, error_handler));
	} catch (LuaError &e) {
		server->setAsyncFatalError(
				std::string("on_emerge_area_completion: ") + e.what() + "\n"
				+ script_get_backtrace(L));
	}

	lua_pop(L, 1); // Pop error handler

	if (state->refcount == 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, state->callback_ref);
		luaL_unref(L, LUA_REGISTRYINDEX, state->args_ref);
	}
}
