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

#include "stack_trace_print.h"
#include "../log.h"

#include <execinfo.h>
#include <iostream>
#include <cstdlib>

std::string GET_STACK_TRACE() {
	constexpr int max = 50;
	void *trace[max];
	uint8_t size = backtrace(trace, max);
	char **symbols = backtrace_symbols(trace, (int)size);
	
	//Make a string and then return
	std::string trace_string = "\nBack trace:";
	for (uint8_t i = 0; i < size; ++i) {
		trace_string += symbols[i];
		trace_string += "\n";
	}
	return trace_string;
}











