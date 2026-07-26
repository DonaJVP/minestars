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

#include "definitions_jumper.h"

#include "stack_trace_print.h"
#include "../network/networkpacket.h"
#include "../log.h"
#include "../ServerNetworkEngine.h"
#include "serialization.h"

#include <deque>
#include <set>

/* __SRV_ITEM_DEF_ENGINE */

node_ident __SRV_ITEM_DEF_ENGINE::containsNodeNAME(const char *name) {
	if (!QUERIED_ALL) {
		warningstream << FUNCTION_NAME << " called with not a set of Items (Overload?)" << std::endl;
		node_ident ni = {false, 0};
		return ni;
	}
	content_t ID;
	bool success = node_def->getId(std::string(name), ID);
	if (success) {
		node_ident n = {true, ID};
		return n;
	}
	node_ident ni = {false, 0};
	return ni;
}

bool __SRV_ITEM_DEF_ENGINE::containsItemNAME(const char *name) {
	if (!QUERIED_ALL) {
		warningstream << FUNCTION_NAME << " called with not a set of Items (Overload?)" << std::endl;
		return false;
	}
	return item_def->isKnown(std::string(name));
}

bool __SRV_ITEM_DEF_ENGINE::hasThisID(uint16_t ID) {
	if (!QUERIED_ALL) {
		warningstream << FUNCTION_NAME << " called with not a set of Items (Overload?)" << std::endl;
		return false;
	}
	return node_def->get(ID).name != "";
}

/* SlaveItemDef */

void SlaveItemDef::registerServerId(uint16_t SRV) {
	ACTIVATED_SERVERS.insert(SRV);
	__SRV_ITEM_DEF_ENGINE *S_ = new __SRV_ITEM_DEF_ENGINE(SRV);
	SlaveServerItemDef[SRV] = S_;
}

void SlaveItemDef::itemDefReceived(uint16_t SRV, std::istream &is) {
	if (ACTIVATED_SERVERS.find(SRV) == ACTIVATED_SERVERS.end()) {
		warningstream << "Got ItemDef but server are not registered [Unknown server id: " << SRV << "]" << std::endl;
		return;
	}
	
	verbosestream << "SlaveItemDef::itemDefReceived(): Received ItemDef from slave:" << SRV << ", Registering...." << std::endl;
	
	__SRV_ITEM_DEF_ENGINE *manager = SlaveServerItemDef[SRV];
	
	manager->item_def->deSerialize(is);
}

void SlaveItemDef::nodeDefReceived(uint16_t SRV, std::istream &is) {
	if (ACTIVATED_SERVERS.find(SRV) == ACTIVATED_SERVERS.end()) {
		warningstream << "Got NodeDef but server are not registered [Unknown server id: " << SRV << "]" << std::endl;
		return;
	}
	
	verbosestream << "SlaveItemDef::nodeDefReceived(): Received ItemDef from slave:" << SRV << ", Registering...." << std::endl;
	
	__SRV_ITEM_DEF_ENGINE *manager = SlaveServerItemDef[SRV];
	
	manager->node_def->deSerialize(is);
}

void SlaveItemDef::startTransformation(uint16_t SRV) {
	if (ACTIVATED_SERVERS.find(SRV) == ACTIVATED_SERVERS.end()) {
		warningstream << "Going to do startTransformation but server are not registered [Unknown server id: " << SRV << "]" << std::endl;
		return;
	}
	
	verbosestream << "Going to check Definitions of the slave node: " << SRV << std::endl;
	
	__SRV_ITEM_DEF_ENGINE *manager = SlaveServerItemDef[SRV];
	//Freeze m_server object
	SNE->FreezeServer(); //m_server->DEFINITIONS_EXECUTION.lock;
	//Start with item def
	std::set<std::string> items_def;
	std::vector<ContentFeatures> nodes_def;
	
	std::unordered_map<content_t, content_t> Jumper;
	std::unordered_map<content_t, content_t> RawJumper;
	
	manager->item_def->getAll(items_def);
	manager->node_def->getAll(nodes_def);
	
	u32 items_added = 0;
	
	for (std::set<std::string>::iterator ITR = items_def.begin(); ITR != items_def.end(); ITR++) {
		//*ITR
		//m_itemdef
		//Check if that item are registered in our inventory
		if (!m_itemdef->isKnown(std::string(*ITR))) {
			verbosestream << "Found new item '" << *ITR << "' from slave " << SRV << "!" << std::endl;
			ItemDefinition item = manager->item_def->get(*ITR);
			m_itemdef->registerItem(item);
			items_added++;
		}
	}
	verbosestream << "Done! <ItemDefinitions>\nGoing to check content_ids" << std::endl;
	
	
	//The slave should register all again
	//Now check if theres any node that has same name but different id, to avoid some bugs: https://www.youtube.com/watch?v=I2b6OARn3Tw
	for (u32 i = 0; i < nodes_def.size(); i++) {
		if (i != CONTENT_IGNORE || i != CONTENT_AIR || i != CONTENT_UNKNOWN) {
			content_t id_of_node_in_main_server = 0;
			const ContentFeatures slave_server_node = nodes_def[i];
			if (m_nodedef->getId(slave_server_node.name, id_of_node_in_main_server)) { //If false, means that the node was not found, but it was already handled by Jumper[]
				Jumper[i] = id_of_node_in_main_server;
				TRACESTREAM(<< "Queued jumpID<already registered node> to node " << slave_server_node.name << " of the slave " << SRV << std::endl);
			} else { //register here and set into the jumper
				if (slave_server_node.name == "")
					continue; //FIXME
				content_t NEW_NODE_ID = m_nodedef->set(slave_server_node.name, slave_server_node);
				Jumper[i] = NEW_NODE_ID; //This will be sent later to the slave to modify the content_t
				//verbosestream << "Queued jumpID<new node> to node " << slave_server_node.name << " of the slave " << SRV << std::endl;
			}
		}
	}
	
	verbosestream << "Done! <NodeDefinitions> [Nodes to modify: " << Jumper.size() << "; Items added to proxy: " << items_added << "]" << std::endl;
	
	infostream << "Definitions of the slave '" << SRV << "' done, going to send modifications into it" << std::endl;
	
	SNE->unFreezeServer();
	
	//Form packet with some responses
	/*
		[u16] Nodes to modify on the slave
		{
			[u16] Node used to be modified
			[u16] New ID for the node {scalable}
		}
		[u16] Nodes to delete and register again with the registered id
		{
			[u16] Node used to be modified
			[u16] New ID for the node
		}
	*/
	NetworkPacket pkt(0x98, 0);
	pkt << (u16)Jumper.size();
	for (auto it = Jumper.begin(); it != Jumper.end(); it++) {
		pkt << it->first << it->second;
	}
	
	/* Send a same-a-like serialize() option to flush all nodes and set how the proxy wants 
	
	std::ostringstream os(std::ios::binary);
	//Put all info here
	
	//Version
	writeU8(os, 1);
	//Count
	u16 count = 0;
	std::ostringstream os2(std::ios::binary);
	for (u32 i = 0; i < nodes_def.size(); i++) {
		if (i == CONTENT_IGNORE || i == CONTENT_AIR || i == CONTENT_UNKNOWN)
			continue;
		if (Jumper.find(i) != Jumper.end()) {
			//Use Jumper
			content_t ID = Jumper[i];
			
			writeU16(os2, ID);
			
			const ContentFeatures *f = &nodes_def[i];
			
			std::ostringstream wrapper_os(std::ios::binary);
			f->serialize(wrapper_os, 38);
			os2<<serializeString16(wrapper_os.str());
			
			if ((count+1) < count)
				break;
			count++;
		}
	}
	writeU16(os, count);
	os << serializeString32(os2.str());
	
	//Compress ZLIB
	std::ostringstream osx2(std::ios::binary);
	compressZlib(os.str(), osx2);
	
	pkt.putLongString(osx2.str());
	*/
	
	//Send nodes and items to register and unregister
	//Slave must raw register all
	
	SNE->SendRawToSlaveServer(SRV, pkt);
}




