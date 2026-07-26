/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, Donatto J. Viveros. P. <donatto555@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include "../threading/thread.h"
#include "../server.h"

class ServerMapFilesSaver: public Thread {
public:
	ServerMapFilesSaver(Server *serv): Thread("SMFS"), m_server(serv) {}
	void saveAllPlayersPositions();
	void loadFiles();
	void init();
	void *run();
	std::atomic<int16_t> clock0;
private:
	Server *m_server = nullptr;
	MultithreadMap<std::string, PlayerDataOMM> *Players;
	std::string m_path_world;
};
