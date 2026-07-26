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

void Server::setSky(RemotePlayer *player, const SkyboxParams &params) {
	sanity_check(player);
	player->setSky(params);
	SendSetSky(player->getPeerId(), params);
}

void Server::setSun(RemotePlayer *player, const SunParams &params) {
	sanity_check(player);
	player->setSun(params);
	SendSetSun(player->getPeerId(), params);
}


void Server::setMoon(RemotePlayer *player, const MoonParams &params) {
	sanity_check(player);
	player->setMoon(params);
	SendSetMoon(player->getPeerId(), params);

}

void Server::setStars(RemotePlayer *player, const StarParams &params) {
	sanity_check(player);
	player->setStars(params);
	SendSetStars(player->getPeerId(), params);

}

void Server::setClouds(RemotePlayer *player, const CloudParams &params) {
	sanity_check(player);
	player->setCloudParams(params);
	SendCloudParams(player->getPeerId(), params);
}
void Server::overrideDayNightRatio(RemotePlayer *player, bool do_override, float ratio) {
	sanity_check(player);
	player->overrideDayNightRatio(do_override, ratio);
	SendOverrideDayNightRatio(player->getPeerId(), do_override, ratio);
}


