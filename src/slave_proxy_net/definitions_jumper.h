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

#include <unordered_map>
#include <set>
#include <sstream>
#include <iostream>

#include "../itemdef.h"
#include "../nodedef.h"

class ServerNetworkEngine;

struct node_ident {
	bool bool_;
	content_t id = 0;
};

class __SRV_ITEM_DEF_ENGINE {
public:
	__SRV_ITEM_DEF_ENGINE(uint16_t S_): _SRV_ID(S_), item_def(createItemDefManager()), node_def(createNodeDefManager()) { }
	uint16_t getThisServerID() { return _SRV_ID; }
	node_ident containsNodeNAME(const char *name);
	bool containsItemNAME(const char *name);
	bool hasThisID(uint16_t ID);
	IWritableItemDefManager *item_def; //Item def of the slave
	NodeDefManager *node_def; //too.
	ServerNetworkEngine *SNE = nullptr;
private:
	uint16_t _SRV_ID = 0;
	bool QUERIED_ALL = false;
};

//Should be retrieved at once in the ServerNetworkEngine class
class SlaveItemDef {
public:
	SlaveItemDef() = default;
	void itemDefReceived(uint16_t SRV, std::istream &is); //sets item_def of the __SRV_ITEM_DEF_ENGINE
	void nodeDefReceived(uint16_t SRV, std::istream &is); //the same
	void registerServerId(uint16_t SRV); //alloc to SlaveServerItemDef & ACTIVATED_SERVERS
	
	void set_itemdef(IWritableItemDefManager *I) { m_itemdef = I; }
	void set_nodedef(NodeDefManager *N) { m_nodedef = N; }
	void startTransformation(uint16_t SRV);
	
	void setSNE(ServerNetworkEngine *sne) { SNE = sne; }
private:
	std::unordered_map<uint16_t, __SRV_ITEM_DEF_ENGINE*> SlaveServerItemDef;
	std::set<uint16_t> ACTIVATED_SERVERS;
	IWritableItemDefManager *m_itemdef = nullptr; //Used to compare and register items of a slave server
	NodeDefManager *m_nodedef = nullptr; //too.
	ServerNetworkEngine *SNE = nullptr;
};









