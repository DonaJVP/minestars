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

#include <unordered_map>

/*
Usage:
BoolField *Verif = new BoolField<uint16_t>();
Verif[12] -> true or false (Default: false)
*/

template<typename _FIELD>
class BoolField {
public:
	BoolField() = default;
	bool at(_FIELD x) { return ((_r.find(x) != _r.end()) ? _r[x] : false); };
	void define(_FIELD x, bool b_) { _r[x] = b_; };
	bool& operator[](_FIELD f) {
		return _r[f];
	}
	const bool& operator[](_FIELD x) const { return at(x); }
	bool switch_values(_FIELD x) { 
		bool _b = at(x);
		if (_b)
			define(x, false);
		else
			define(x, true);
		return at(x);
	}
private:
	std::unordered_map<_FIELD, bool> _r;
};
