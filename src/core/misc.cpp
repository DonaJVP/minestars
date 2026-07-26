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

#include "../translation.h"
#include "../server.h"
#include "../nodedef.h"
#include "../itemdef.h"
#include "../craftdef.h"
#include "../filesys.h"
//#include "../content/mods.h" // Obsolete
#include "../addons/addons.hpp"
#include "../server/mods.h"
#include "../rollback.h"
#include "../rollback_interface.h"

Translations *Server::getTranslationLanguage(const std::string &lang_code)
{
    if (lang_code.empty())
        return nullptr;

    auto it = server_translations.find(lang_code);
    if (it != server_translations.end())
        return &it->second; // Already loaded

        // [] will create an entry
    auto *translations = &server_translations[lang_code];

    std::string suffix = "." + lang_code + ".tr";
    for (const auto &i : m_media) {
        if (str_ends_with(i.first, suffix)) {
            std::string data;
            if (fs::ReadFile(i.second.path, data)) {
                translations->loadTranslation(data);
            }
        }
    }

    return translations;
}

// IGameDef interface
// Under envlock
IItemDefManager *Server::getItemDefManager()
{
    return m_itemdef;
}

const NodeDefManager *Server::getNodeDefManager()
{
    return m_nodedef;
}

ICraftDefManager *Server::getCraftDefManager()
{
    return m_craftdef;
}

u16 Server::allocateUnknownNodeId(const std::string &name)
{
    return m_nodedef->allocateDummy(name);
}

IWritableItemDefManager *Server::getWritableItemDefManager()
{
    return m_itemdef;
}

NodeDefManager *Server::getWritableNodeDefManager()
{
    return m_nodedef;
}

IWritableCraftDefManager *Server::getWritableCraftDefManager()
{
    return m_craftdef;
}
/*
const std::vector<ModSpec> & Server::getMods() const
{
    return m_modmgr->getMods();
}

const ModSpec *Server::getModSpec(const std::string &modname) const
{
    return m_modmgr->getModSpec(modname);
}

void Server::getModNames(std::vector<std::string> &modlist)
{
    m_modmgr->getModNames(modlist);
}
*/
std::string Server::getBuiltinLuaPath()
{
    return porting::path_share + DIR_DELIM + "builtin";
}
