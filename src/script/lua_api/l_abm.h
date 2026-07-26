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

#pragma once

#include "lua_api/l_base.h"

class ModApiAbmT : public ModApiBase
{
private: 
	static int l_set_node(lua_State *L);
	static int l_get_node(lua_State *L);
	static int l_swap_node(lua_State *L);
	static int l_sound_play(lua_State *L);
	static int l_remove_node(lua_State *L);
	static int l_is_protected(lua_State *L);
	static int l_get_node_timer(lua_State *L);
	static int l_find_node_near(lua_State *L);
	static int l_get_node_light(lua_State *L);
	static int l_get_node_or_nil(lua_State *L);
	static int l_find_nodes_in_area(lua_State *L);
	static int l_find_nodes_in_area_under_air(lua_State *L);
public:
	static void Initialize(lua_State *L, int top);
protected:
	static void collectNodeIds(lua_State *L, int idx, const NodeDefManager *ndef, std::vector<content_t> &filter);
	static void checkArea(v3s16 &minp, v3s16 &maxp);
};