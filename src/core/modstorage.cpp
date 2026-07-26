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
#include "../log.h"
#include "../filesys.h"

#include <string>

std::string Server::getModStoragePath() const
{
    return m_path_world + DIR_DELIM + "mod_storage";
}

bool Server::registerModStorage(ModMetadata *storage) {
    if (m_mod_storages.find(storage->getModName()) != m_mod_storages.end()) {
        errorstream << "Unable to register same mod storage twice. Storage name: "
        << storage->getModName() << std::endl;
        return false;
    }

    m_mod_storages[storage->getModName()] = storage;
    return true;
}

void Server::unregisterModStorage(const std::string &name) {
    std::unordered_map<std::string, ModMetadata *>::const_iterator it = m_mod_storages.find(name);
    if (it != m_mod_storages.end()) {
        // Save unconditionaly on unregistration
        it->second->save(getModStoragePath());
        m_mod_storages.erase(name);
    }
}
