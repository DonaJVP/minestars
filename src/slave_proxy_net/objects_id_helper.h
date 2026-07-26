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

//Made for the Proxy, like SuperIDs without that much bloat

#include <unordered_map>
#include <cstdint>

class ids {
public:
	void registerid(uint16_t id_r, uint16_t id_f) {
		IDS[id_r] = id_f;
	}
	void unregisterid(uint16_t id) {
		if (IDS.find(id) != IDS.end()) {
			IDS.erase(id);
		}
	}
	uint16_t getid(uint16_t id, bool &status) {
		status = IDS.find(id) != IDS.end();
		return IDS.find(id) == IDS.end() ? 0 : IDS[id];
	}
private:
	std::unordered_map<uint16_t, uint16_t> IDS;
};
