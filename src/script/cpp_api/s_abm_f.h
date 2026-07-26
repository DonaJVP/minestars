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

#include <vector>
#include <deque>
#include <unordered_set>
#include <memory>

#include <lua.h>
#include "threading/semaphore.h"
#include "threading/thread.h"
#include "common/c_packer.h"
#include "cpp_api/s_base.h"
#include "environment.h"
#include "mapnode.h"
#include "cpp_api/s_security.h"

class ABME;

struct ABME_data
{
	ABME_data() = default;
	std::string function;
	u32 aoc;
	u32 aocw;
	v3s16 pos;
	MapNode node;
};

class ABMT : public Thread, virtual public ScriptApiBase, public ScriptApiSecurity {
	friend class ABME;
public:
	virtual ~ABMT();
	void *run();
protected:
	ABMT(ABME* jobDispatcher, const std::string &name);
private:
	ABME *jobDispatcher = nullptr;
	bool error = false;
};

class ABME {
	friend class ABMT;
	typedef void (*StateInitializer)(lua_State *L, int top);
public:
	ABME() = default;
	ABME(Server *server) : server(server) {};
	~ABME();
	void reg_init_l(StateInitializer func);
	void init(); //select all threads
	bool QueueABM(std::string &&function, v3s16 p, MapNode n, u32 aoc, u32 aocw);
	void step();
private:
	u64 ast = 0;
	std::mutex abmsMutex;
	bool init_done = false;
	Server *server = nullptr;
	std::deque<ABME_data> abms;
	std::vector<ABMT*> threads;
	std::unordered_set<u32> asj;
	unsigned int maxthreads = 0;
	std::vector<StateInitializer> initializers;
	Semaphore smp;
protected:
	void scale();
	void addthread();
	bool doenv(lua_State* L, int top);
	bool getavailableabm(ABME_data *job);
};










