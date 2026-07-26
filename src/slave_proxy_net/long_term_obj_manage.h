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

#include <mutex>
#include <unordered_map>

#pragma once

//Eye by eye, this should be only used in ServersNetworkEngine for each slave server

enum {
	NOT_KNOWN_PLAYER_ID = UINT16_MAX-1,
	UNKNOWN_OBJECT_ID = UINT16_MAX-2,
	PLAYER_OBJECT_ID = UINT16_MAX-3,
	KNOWN_OBJECT_ID = UINT16_MAX-4,
};
typedef uint16_t _player;
typedef uint16_t _obj;
typedef std::unordered_map<uint16_t, uint16_t> _u16_map;
typedef std::unordered_map<uint16_t, _u16_map> _2u16_map;

class SlaveServerObjects {
public:
	SlaveServerObjects() = default;
	_obj GetObjectByPlayer(_player player, _obj in_slave_id); //Acquires the object ID of the player vision else returns UNKNOWN_OBJECT_ID
	const _u16_map GetObjectsMapFromPlayer(_player player); //Must not be writable
	_obj RegisterObjectForPlayer(_player player, _obj object);
	bool UnRegisterObjectForPlayer(_player player, _obj object);
	void ClearPlayerVision(_player player); //Used only when player leaves a slave
	bool RegisterPlayer(_player player, uint16_t player_sao_id_on_player_vision, uint16_t player_sao_on_slave); //Slave should send a unique SAO id for us
	bool EqualsSAOidWITHgivenRes(_player player, _obj obj);
private:
	_2u16_map PlayerVision; //Each player will had his own map of objects
	_u16_map InSlavePlayerSaoIDs;
	std::mutex _mutex; //This have been used in multithread environment
};
