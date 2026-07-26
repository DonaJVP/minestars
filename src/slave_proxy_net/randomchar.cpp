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

#include <random>
#include <string>
#include "randomchar.h"

const std::string _LETTERS = "abcdefghijklmnopqrstuvwyxz123456789";
static std::random_device _RD;
static std::mt19937 _GEN(_RD());

uint32_t randomNumber(uint32_t start, uint32_t end) {
	std::uniform_int_distribution<uint32_t> dist(start, end-1);
	return dist(_GEN);
}

std::string randomString(uint16_t lenght) {
	std::string _tr;
	_tr.reserve(lenght);
	for (uint16_t i = 0; i < lenght; i++) {
		//Get a random number
		char c = _LETTERS[randomNumber(0, _LETTERS.size())];
		_tr += c;
	}
	return _tr;
}