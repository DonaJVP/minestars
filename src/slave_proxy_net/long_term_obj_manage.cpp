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

// Long Term Obj Manager, for each slave server & clients
// This will help slave servers to link all objects from IDs to other ID registered by this
// Also, will help managing the user experience for objects

#include "long_term_obj_manage.h"
#include "randomchar.h" //For number chose for ids
#include <mutex>
#include <iostream>
#include "../log.h"

//Gets the registered ID of slave translated into player vision
_obj SlaveServerObjects::GetObjectByPlayer(_player player, _obj in_slave_id) {
	_mutex.lock();
	if (PlayerVision.find(player) != PlayerVision.end()) {
		if (PlayerVision.at(player).find(in_slave_id) != PlayerVision.at(player).end()) {
			//Return the registered object
			_obj _i = PlayerVision.at(player).at(in_slave_id);
			_mutex.unlock();
			return _i;
		} else if (InSlavePlayerSaoIDs.find(in_slave_id) != InSlavePlayerSaoIDs.end()) { //Means that something has required a ID which is from a player
			_mutex.unlock();
			return PLAYER_OBJECT_ID;
		} else {
			_mutex.unlock();
			return UNKNOWN_OBJECT_ID;
		}
	} else {
		_mutex.unlock();
		return NOT_KNOWN_PLAYER_ID;
	}
}

const _u16_map SlaveServerObjects::GetObjectsMapFromPlayer(_player player) {
	_mutex.unlock();
	if (PlayerVision.find(player) != PlayerVision.end()) {
		_u16_map _p = PlayerVision.at(player);
		_mutex.unlock();
		return _p;
	}
	_mutex.unlock();
	return _u16_map();
}

_obj SlaveServerObjects::RegisterObjectForPlayer(_player player, _obj object) {
	_mutex.lock();
	if (PlayerVision.find(player) != PlayerVision.end()) {
		if (PlayerVision.at(player).find(object) == PlayerVision.at(player).end()) {
			const _u16_map _map = GetObjectsMapFromPlayer(player);
			auto exists = [_map] (_obj ID, bool &res) {
				if (_map.find(ID) == _map.end()) {
					return true;
				} else {
					return false;
				}
			};
			bool result = false;
			_obj _ID = 0;
			while (!result) {
				_ID = (_obj)randomNumber(1, UINT16_MAX-5);
				result = exists(_ID, result);
				verbosestream << "SlaveServerObjects::RegisterObjectForPlayer: SearchID: " << _ID << ", found: " << result << std::endl;
			}
			PlayerVision.at(player)[object] = _ID; //Register
			_mutex.unlock();
			return _ID; //Send
		} else {
			_mutex.unlock();
			return KNOWN_OBJECT_ID; //It is already registered
		}
	} else {
		_mutex.unlock();
		return NOT_KNOWN_PLAYER_ID;
	}
}

bool SlaveServerObjects::UnRegisterObjectForPlayer(_player player, _obj object) {
	_mutex.lock();
	if (PlayerVision.find(player) != PlayerVision.end()) {
		if (PlayerVision.at(player).find(object) != PlayerVision.at(player).end()) {
			PlayerVision.at(player).erase(object);
			_mutex.unlock();
			return true;
		} else {
			_mutex.unlock();
			return false;
		}
	} else {
		_mutex.unlock();
		return false;
	}
}

void SlaveServerObjects::ClearPlayerVision(_player player) {
	_mutex.lock();
	if (PlayerVision.find(player) != PlayerVision.end()) {
		PlayerVision.at(player).clear();
		PlayerVision.erase(player);
	}
	_mutex.unlock();
}

bool SlaveServerObjects::RegisterPlayer(_player player, uint16_t psidopv, uint16_t psos) {
	_mutex.lock();
	if (PlayerVision.find(player) == PlayerVision.end()) {
		PlayerVision[player] = _u16_map();
		PlayerVision.at(player)[psos] = psidopv;
		InSlavePlayerSaoIDs[psos] = psidopv;
		_mutex.unlock();
		return true;
	} else {
		_mutex.unlock();
		return false;
	}
}

bool SlaveServerObjects::EqualsSAOidWITHgivenRes(_player player, _obj obj) {
	_mutex.lock();
	if (PlayerVision.find(player) == PlayerVision.end()) {
		if (InSlavePlayerSaoIDs.find(player) != InSlavePlayerSaoIDs.end()) {
			_mutex.unlock();
			return InSlavePlayerSaoIDs.at(player) == obj;
		}
	}
	_mutex.unlock();
	return false;
}




















