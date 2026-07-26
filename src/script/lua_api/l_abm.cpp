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
#include "lua_api/l_abm.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_nodetimer.h"
#include "lua_api/l_noise.h"
#include "lua_api/l_vmanip.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "environment.h"
#include "mapblock.h"
#include "server.h"
#include "nodedef.h"
#include "daynightratio.h"
#include "mapgen/treegen.h"
#include "emerge_internal.h"
#include "face_position_cache.h"
#include "server/luaentity_sao.h"
#include "server/player_sao.h"
#include "util/string.h"
#include "translation.h"

int ModApiAbmT::l_set_node(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;
	const NodeDefManager *ndef = env->getGameDef()->ndef();
	// parameters
	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2, ndef);
	// Do it
	env->QueueNodeModify(0, pos, n, true);
	lua_pushboolean(L, true);
	return 1;
}

int ModApiAbmT::l_get_node(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;
	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	MapNode n = env->getMap().getNode(pos);
	// Return node
	pushnode(L, n, env->getGameDef()->ndef());
	return 1;
}

int ModApiAbmT::l_swap_node(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	// parameters
	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2, ndef);
	// Do it
	env->QueueNodeModify(2, pos, n, true);
	lua_pushboolean(L, true);
	return 1;
}

int ModApiAbmT::l_remove_node(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;

	// parameters
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	env->QueueNodeModify(1, pos, NULL, true);
	lua_pushboolean(L, true);
	return 1;
}

int ModApiAbmT::l_get_node_or_nil(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;

	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	bool pos_ok;
	MapNode n = env->getMap().getNode(pos, &pos_ok);
	if (pos_ok) {
		// Return node
		pushnode(L, n, env->getGameDef()->ndef());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ModApiAbmT::l_get_node_light(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;

	// Do it
	v3s16 pos = read_v3s16(L, 1);
	u32 time_of_day = env->getTimeOfDay();
	if(lua_isnumber(L, 2))
		time_of_day = 24000.0 * lua_tonumber(L, 2);
	time_of_day %= 24000;
	u32 dnr = time_to_daynight_ratio(time_of_day, true);

	bool is_position_ok;
	MapNode n = env->getMap().getNode(pos, &is_position_ok);
	if (is_position_ok) {
		const NodeDefManager *ndef = env->getGameDef()->ndef();
		lua_pushinteger(L, n.getLightBlend(dnr, ndef));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ModApiAbmT::l_is_protected(lua_State *L) {
	//Abm can modify everything, should return true at all, no?
	lua_pushboolean(L, true);
	return 1;
}

int ModApiAbmT::l_sound_play(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	ServerSoundParams params;
	read_server_sound_params(L, 2, params);
	bool ephemeral = lua_gettop(L) > 2 && readParam<bool>(L, 3);
	if (ephemeral) {
		getServer(ModApiBase::CCL)->playSound(spec, params, true);
		lua_pushnil(L);
	} else {
		s32 handle = getServer(ModApiBase::CCL)->playSound(spec, params);
		lua_pushinteger(L, handle);
	}
	return 1;
}

int ModApiAbmT::l_find_node_near(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

	v3s16 pos = read_v3s16(L, 1);
	int radius = luaL_checkinteger(L, 2);
	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	int start_radius = (lua_isboolean(L, 4) && readParam<bool>(L, 4)) ? 0 : 1;

	for (int d = start_radius; d <= radius; d++) {
		const std::vector<v3s16> &list = FacePositionCache::getFacePositions(d);
		for (const v3s16 &i : list) {
			v3s16 p = pos + i;
			content_t c = map.getNode(p).getContent();
			if (CONTAINS(filter, c)) {
				push_v3s16(L, p);
				return 1;
			}
		}
	}
	return 0;
}

int ModApiAbmT::l_find_nodes_in_area(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;

	v3s16 minp = read_v3s16(L, 1);
	v3s16 maxp = read_v3s16(L, 2);
	sortBoxVerticies(minp, maxp);

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

	checkArea(minp, maxp);

	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	bool grouped = lua_isboolean(L, 4) && readParam<bool>(L, 4);

	if (grouped) {
		// create the table we will be returning
		lua_createtable(L, 0, filter.size());
		int base = lua_gettop(L);

		// create one table for each filter
		std::vector<u32> idx;
		idx.resize(filter.size());
		for (u32 i = 0; i < filter.size(); i++)
			lua_newtable(L);

		v3s16 p;
		for (p.X = minp.X; p.X <= maxp.X; p.X++)
		for (p.Y = minp.Y; p.Y <= maxp.Y; p.Y++)
		for (p.Z = minp.Z; p.Z <= maxp.Z; p.Z++) {
			content_t c = map.getNode(p).getContent();

			auto it = std::find(filter.begin(), filter.end(), c);
			if (it != filter.end()) {
				// Calculate index of the table and append the position
				u32 filt_index = it - filter.begin();
				push_v3s16(L, p);
				lua_rawseti(L, base + 1 + filt_index, ++idx[filt_index]);
			}
		}

		// last filter table is at top of stack
		u32 i = filter.size() - 1;
		do {
			if (idx[i] == 0) {
				// No such node found -> drop the empty table
				lua_pop(L, 1);
			} else {
				// This node was found -> put table into the return table
				lua_setfield(L, base, ndef->get(filter[i]).name.c_str());
			}
		} while (i-- != 0);

		assert(lua_gettop(L) == base);
		return 1;
	} else {
		std::vector<u32> individual_count;
		individual_count.resize(filter.size());

		lua_newtable(L);
		u32 i = 0;
		v3s16 p;
		for (p.X = minp.X; p.X <= maxp.X; p.X++)
		for (p.Y = minp.Y; p.Y <= maxp.Y; p.Y++)
		for (p.Z = minp.Z; p.Z <= maxp.Z; p.Z++) {
			content_t c = env->getMap().getNode(p).getContent();

			auto it = std::find(filter.begin(), filter.end(), c);
			if (it != filter.end()) {
				push_v3s16(L, p);
				lua_rawseti(L, -2, ++i);

				u32 filt_index = it - filter.begin();
				individual_count[filt_index]++;
			}
		}

		lua_createtable(L, 0, filter.size());
		for (u32 i = 0; i < filter.size(); i++) {
			lua_pushinteger(L, individual_count[i]);
			lua_setfield(L, -2, ndef->get(filter[i]).name.c_str());
		}
		return 2;
	}
}

int ModApiAbmT::l_find_nodes_in_area_under_air(lua_State *L)
{
	/* Note: A similar but generalized (and therefore slower) version of this
	 * function could be created -- e.g. find_nodes_in_area_under -- which
	 * would accept a node name (or ID?) or list of names that the "above node"
	 * should be.
	 * TODO
	 */

	ServerEnvironment *env = ModApiBase::ENV;

	v3s16 minp = read_v3s16(L, 1);
	v3s16 maxp = read_v3s16(L, 2);
	sortBoxVerticies(minp, maxp);

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

#ifndef SERVER
	if (Client *client = getClient(L)) {
		minp = client->CSMClampPos(minp);
		maxp = client->CSMClampPos(maxp);
	}
#endif

	checkArea(minp, maxp);

	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	lua_newtable(L);
	u32 i = 0;
	v3s16 p;
	for (p.X = minp.X; p.X <= maxp.X; p.X++)
	for (p.Z = minp.Z; p.Z <= maxp.Z; p.Z++) {
		p.Y = minp.Y;
		content_t c = map.getNode(p).getContent();
		for (; p.Y <= maxp.Y; p.Y++) {
			v3s16 psurf(p.X, p.Y + 1, p.Z);
			content_t csurf = map.getNode(psurf).getContent();
			if (c != CONTENT_AIR && csurf == CONTENT_AIR &&
					CONTAINS(filter, c)) {
				push_v3s16(L, p);
				lua_rawseti(L, -2, ++i);
			}
			c = csurf;
		}
	}
	return 1;
}

int ModApiAbmT::l_get_node_timer(lua_State *L)
{
	ServerEnvironment *env = ModApiBase::ENV;

	// Do it
	v3s16 p = read_v3s16(L, 1);
	NodeTimerRef::create(L, p, &env->getServerMap());
	return 1;
}

/* Helpers */
void ModApiAbmT::checkArea(v3s16 &minp, v3s16 &maxp)
{
	auto volume = VoxelArea(minp, maxp).getVolume();
	// Volume limit equal to 8 default mapchunks, (80 * 2) ^ 3 = 4,096,000
	if (volume > 4096000) {
		throw LuaError("Area volume exceeds allowed value of 4096000");
	}

	// Clamp to map range to avoid problems
#define CLAMP(arg) core::clamp(arg, (s16)-MAX_MAP_GENERATION_LIMIT, (s16)MAX_MAP_GENERATION_LIMIT)
	minp = v3s16(CLAMP(minp.X), CLAMP(minp.Y), CLAMP(minp.Z));
	maxp = v3s16(CLAMP(maxp.X), CLAMP(maxp.Y), CLAMP(maxp.Z));
#undef CLAMP
}
void ModApiAbmT::collectNodeIds(lua_State *L, int idx, const NodeDefManager *ndef,
	std::vector<content_t> &filter)
{
	if (lua_istable(L, idx)) {
		lua_pushnil(L);
		while (lua_next(L, idx) != 0) {
			// key at index -2 and value at index -1
			luaL_checktype(L, -1, LUA_TSTRING);
			ndef->getIds(readParam<std::string>(L, -1), filter);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	} else if (lua_isstring(L, idx)) {
		ndef->getIds(readParam<std::string>(L, 3), filter);
	}
}

/* Initialize */

void ModApiAbmT::Initialize(lua_State *L, int top)
{
	API_FCT(set_node);
	API_FCT(get_node);
	API_FCT(swap_node);
	API_FCT(sound_play);
	API_FCT(remove_node);
	API_FCT(is_protected);
	API_FCT(get_node_timer);
	API_FCT(find_node_near);
	API_FCT(get_node_light);
	API_FCT(get_node_or_nil);
	API_FCT(find_nodes_in_area);
	API_FCT(find_nodes_in_area_under_air);
}