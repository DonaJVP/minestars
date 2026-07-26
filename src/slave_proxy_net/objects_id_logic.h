/*
MineStars - MultiCraft - Minetest/Luanti
Copyright (C) 2025 Logiki, <donatto555@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

//This should be used on every player as an new object? YES.
//I will use uint16_t because i want.

#pragma once

#include <unordered_map>
#include <cstdint>

typedef std::unordered_map<uint16_t, uint16_t> unit_map;

class MasterServerUniqueIDS {
public:
	MasterServerUniqueIDS(uint16_t ID); //register
	uint16_t getUniqueID(uint16_t ID, uint16_t S_P_ID); //Translates ID to an registered id, if the S_P_ID is the same as ID returns Player real ID (may used for body changes, etc)
	uint16_t getUniqueID(uint16_t ID); //Translates ID to a proper ID seen by the player
	uint16_t RgetUniqueID(uint16_t ID); //Reverse
	uint16_t registerUniqueID(uint16_t ID); //Registers an ID of the server and translates it for the server
	void deleteUniqueID(uint16_t ID); //Deletes the registered ID that the registerUniqueID|getUniqueID gives
	uint16_t getPlayerId() { return PLAYERSAO_ID; }
	bool hasThisID(uint16_t ID);
	void SET_SAO_ID(uint16_t i) { PLAYERSAO_ID = i; }
private:
	uint16_t PLAYERSAO_ID;
	uint16_t P_PRX_SAO;
	unit_map REGISTERED_UNIQUE_IDS; //First index: real object id. Second index: index viewed for the player
	unit_map rREGISTERED_UNIQUE_IDS; //this is reverse
protected:
	bool existsID(uint16_t ID) {
		return REGISTERED_UNIQUE_IDS.find(ID) != REGISTERED_UNIQUE_IDS.end();
	}
};
