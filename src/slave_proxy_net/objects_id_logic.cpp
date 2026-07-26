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

#include "objects_id_logic.h" //$THIS
#include "stack_trace_print.h" //GET_STACK_TRACE()
#include "../log.h"

#include <random>
#include <sstream>
#include <iostream>

static void reportWarning(std::string str) {
	warningstream << str << ", \n" << GET_STACK_TRACE() << std::endl;
}

MasterServerUniqueIDS::MasterServerUniqueIDS(uint16_t ID): P_PRX_SAO(ID) {
	actionstream << "Registered new UniqueID storage for player " << ID << std::endl;
}

uint16_t MasterServerUniqueIDS::getUniqueID(uint16_t ID) {
	if (ID == PLAYERSAO_ID)
		return P_PRX_SAO; //Return the ID which the player has originally
	if (existsID(ID)) {
		return REGISTERED_UNIQUE_IDS[ID];
	} else {
		std::stringstream ss;
		ss << "ID for Real ID not found: ";
		ss << ID;
		reportWarning(ss.str());
		return (uint16_t)0;
	}
}

uint16_t MasterServerUniqueIDS::RgetUniqueID(uint16_t ID) {
	if (rREGISTERED_UNIQUE_IDS.find(ID) != rREGISTERED_UNIQUE_IDS.end()) {
		return rREGISTERED_UNIQUE_IDS[ID];
	} else {
		std::stringstream ss;
		ss << "ID for Real ID not found: ";
		ss << ID;
		reportWarning(ss.str());
		return (uint16_t)0;
	}
}

uint16_t MasterServerUniqueIDS::getUniqueID(uint16_t ID, uint16_t S_P_ID) {
	if (ID == S_P_ID)
		return PLAYERSAO_ID;
	if (existsID(ID)) {
		return REGISTERED_UNIQUE_IDS[ID];
	} else {
		std::stringstream ss;
		ss << "ID for Real ID not found: ";
		ss << ID;
		reportWarning(ss.str());
		return (uint16_t)0;
	}
}

static uint16_t getRandomIndex(unit_map *map, uint16_t PID) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<uint16_t> dist(0, UINT16_MAX-1);
	
	bool got_index = false;
	while (!got_index) {
		register uint16_t idx = dist(gen);
		if (map->find(idx) == map->end() && idx != PID) {
			got_index = true;
			return idx;
		}
	}
}

uint16_t MasterServerUniqueIDS::registerUniqueID(uint16_t ID) {
	if (existsID(ID)) {
		reportWarning("ID is already registered, returning the registered ID");
		return REGISTERED_UNIQUE_IDS[ID];
	} else {
		uint16_t n_idx = getRandomIndex(&REGISTERED_UNIQUE_IDS, PLAYERSAO_ID);
		REGISTERED_UNIQUE_IDS[ID] = n_idx;
		rREGISTERED_UNIQUE_IDS[n_idx] = ID;
		return n_idx;
	}
}

void MasterServerUniqueIDS::deleteUniqueID(uint16_t ID) {
	if (existsID(ID)) {
		rREGISTERED_UNIQUE_IDS.erase(REGISTERED_UNIQUE_IDS[ID]);
		REGISTERED_UNIQUE_IDS.erase(ID);
	} else {
		reportWarning("ID does not exist!");
	}
}

bool MasterServerUniqueIDS::hasThisID(uint16_t ID) {
	return REGISTERED_UNIQUE_IDS.find(ID) != REGISTERED_UNIQUE_IDS.end() || ID == PLAYERSAO_ID;
}

















