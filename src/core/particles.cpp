/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, Donatto J. Viveros. P. <donatto555@gmail.com>
 * Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#include "../server.h"
#include "../remoteplayer.h"
#include "../player.h"
#include <cstdint>
#include <string>

void Server::spawnParticle(const std::string &playername, const ParticleParameters &p) {
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return;

	uint16_t peer_id = PEER_ID_INEXISTENT;
	uint16_t proto_ver = 0;
	uint16_t playerid = 0;


	if (!playername.empty()) {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return;
		peer_id = player->getPeerId();
		proto_ver = player->protocol_version;
	}
	SendSpawnParticle(peer_id, proto_ver, p);
}

uint32_t Server::addParticleSpawner(const ParticleSpawnerParameters &p, ServerActiveObject *attached, const std::string &playername)
{
	// m_env will be NULL if the server is initializingaddParticleSpawner
	if (!m_env)
		return -1;

	uint16_t peer_id = PEER_ID_INEXISTENT;
	uint16_t proto_ver = 0;
	uint16_t playerid = 0;
	uint16_t attached_id = attached ? attached->getId() : 0;
	uint32_t id;

	if (!playername.empty()) {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return -1;
		peer_id = player->getPeerId();
		proto_ver = player->protocol_version;
	}
	if (attached_id == 0)
		id = m_env->addParticleSpawner(p.time);
	else
		id = m_env->addParticleSpawner(p.time, attached_id);
	SendAddParticleSpawner(peer_id, proto_ver, p, attached_id, id);
	return id;
}

void Server::deleteParticleSpawner(const std::string &playername, u32 id)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return;

	session_t peer_id = PEER_ID_INEXISTENT;
	uint16_t playerid = 0; //default

	if (!playername.empty()) {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return;
		peer_id = player->getPeerId();
	}
	SendDeleteParticleSpawner(peer_id, id);
	m_env->deleteParticleSpawner(id);
}
