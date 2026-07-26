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

/*
	MineStars Engine Part: Multi Server Engine
	
	Made to simulate BungeeCord and other Proxy engines
	
	Mental notes:
	AT client/client.{cpp, h}: cpp:L_{118, 324}, h:L_(NULL) #See some variables to use
	
	[Proxy] -> (Player Data Simulation) -> _peer_ -> [Selected server]
	NOTES: Those selected servers should be as a *client* of the proxy, 
	as they need to handle direct player information [Auth data is handled only by Proxy]
	
	WARNING: THIS IS AN PROXY SERVER, NOT A IN TO USE SERVER
	How does this warning work?:
	Proxy != Server
	
                                                                                                            
                                                                                                            
     [ Player Interactions ]                                                                                
          |                                                                                                 
          |                                                                                                 
          |                                                                                                 
          v                                                                                                 
     [ Proxy MainLine (Normal engine execution with overrides) ]                                            
          |                                             ^                                                   
          |                                             |                                                   
    ###############################################     |                                                   
    # Are the player on one of the proxy servers? #     |                                                   
    ###############################################     |                                                   
          |                                             |                                                   
          #----------------------#                      |                                                   
          |                      |                      |                                                   
          v                      v                      |                                                   
       #######                #######                   |                                                   
       # Yes #                # No. #-------------------#                                                   
       #######                #######                                                                       
          |                                                                                                 
          |                                                                                                 
          v                                                                                                 
    [-----#---------------------------------------------]                                                   
    [ Packet translator, using uint16_t for player IDs, ]                                                   
    [ not IP:PORT per packet, because it will broke     ]                     (How it works )>-v            
    [ completely the game, as the HimbeerserverDE       ] ---------> [ RemotePlayer *player => player->PlayerID = rand(0, 65535) ]
    [ proxy server.                                     ]                |                                  
    [ Every slave server will need to verify the IP     ]                #--->( Convert Player-Sent Command & Data to only one packet to #Servers )
    [___________________________________________________]                                                   
                |        ^                                                                                  
                |        |->[ Some Environment Updates to players ]                                         
                v        |                                                                                  
                #-->[ Servers ]                                                                             
*/                                                                                                          

/*
	Some creation of threads are needed, to handle incomming actions from the slave servers
	May there needs to be a thread to send actions to players?
*/

#include <iostream>
#include <queue>
#include <map>
#include <algorithm>
#include <atomic>

#include "slave_proxy_net/objects_id_helper.h"
#include "network/mt_connection.h"
#include "network/networkpacket.h"
#include "network/address.h"
#include "network/peerhandler.h"
#include "threading/thread.h"
#include "log.h"
#include "media.h"

#pragma once

#define MAX_FUNC_SCALE 0x77

class ServerEnvironment;
class SlaveItemDef;

// Exception if a fatal failure occurs on server indexing
class FailureOnIndexingARegisteredServer : public std::exception {
public:
	const char* what() const noexcept override {
		errorstream << "An error occurs when trying to index a *registered* server" << std::endl;
		return "An error occurs when trying to index a *registered* server";
	}
};

class PeerHandler__: public con::PeerHandler {
public:
	PeerHandler__() = default;
	void peerAdded(con::Peer *peer) { actionstream << "Peer addded" << std::endl; };
	void deletingPeer(con::Peer *peer, bool timeout) { actionstream << "Quitting a peer" << std::endl; };
};

// Servers registering
struct ServerFrontStructure {
	ServerFrontStructure() = default;
	Address address;
	Address bind_address;
	std::string server_name; //obsolete
	int ID;
};

struct queued_message {
	std::string old_;
	std::string new_;
	u16 id_;
};

class SlaveServerPM;
class ServerNetworkEngine;
class ActiveSSThread;
class SlaveServerObjects;

struct _aom_double {
	_aom_double(u16 player, u16 obj, std::string o, std::string n, u16 cmd, bool reliable): _old(o), _new(n), obj(obj), player(player), cmd(cmd), reliable(reliable) {}
	std::string _old, _new;
	u16 obj;
	u16 player;
	u16 cmd;
	bool reliable;
};

struct ActiveServersStructure {
	ActiveServersStructure(ServerNetworkEngine *SNE);
	ServerFrontStructure *ServerStruct;
	con::Connection *srv_connection = nullptr; //connect&send address
	con::Connection *BINDsrv_connection = nullptr; //bind address
	std::atomic<bool> active;
	u16 ID; //may handle?
	bool independient = false;
	SlaveServerPM *SlaveServerObject = nullptr;
	std::mutex mutex;
	std::deque<NetworkPacket> aomMsgs;
	ServerNetworkEngine *SNE = nullptr;
	ActiveSSThread *thread = nullptr;
	std::unordered_map<u16, bool> players_protonet; //Updated in IN/OUT on SendDisconnect and SendConnect, registering protocols
	std::unordered_map<u16, session_t> players;
	std::unordered_map<u16, u16> reverse_players_sao;
	std::unordered_map<u16, u16> players_sao; //Updated in multiserver_env[.cpp]
	//std::unordered_map<u16, ids> objs_map_clients; //SuperIDs instance for proxy, updates sent by slave
	//std::unordered_map<u16, std::unordered_map<u16, u16>> objects_known_by_clients;
	std::deque<queued_message> queued_messages; //Sometimes the slave server may send a update to an object that the player doens't know, so we will queue it into here
	SlaveServerObjects *SSO = nullptr;

	bool SocketConn = false;
	std::string SocketDir = "";
	bool SocketReady = false;
	uint16_t socket_id = 0;
	std::unordered_map<uint16_t, bool> InitializedPlayers;
};

//A.S.S.T has is own thread [don't read it like a word!]
class ActiveSSThread: public Thread {
public:
	~ActiveSSThread() {};
	void *run();
	ActiveSSThread(ActiveServersStructure *jb): Thread(std::string("ActiveSS")), jb(jb) {};
	SlaveServerObjects *SSO = nullptr;
private:
	ActiveServersStructure *jb = nullptr;
	std::deque<_aom_double> preObjQueue;
};


class SlaveServer {
	public:
		SlaveServer() = default; //Constructor
		bool IsApplied = false; //This is true when a player is in a slave server
		bool InSlaveServerISPLAYINGHERE = false; //useful to get if player are playing in 'this' as an slave instance
		//ServerFrontStructure ServerStruct; //Slave server data
		u16 ID; //Faster option
		explicit operator bool() const { // =(ITM)
			return IsApplied;
		}
		bool operator!() const { // =(!ITM)
			return !IsApplied;
		}
};

class QueuedPacketsHelper {
public:
	QueuedPacketsHelper();
	void doLock() { mutex_.lock(); }
	void doUnLock() { mutex_.unlock(); }
	NetworkPacket *getPacket();
	void QueuePacket(NetworkPacket pkt);
private:
	std::mutex mutex_;
	std::deque<NetworkPacket *> packets; //queued packets
};

class Server;
//struct MediaInfo;

struct SC_H {
	SC_H() = default;
	SC_H(u16 player_id, u16 srv);
	u16 player;
	u16 server;
	s16 time = 0;
	bool phases[3] = {false, false, false}; //lol
};

class _SNE_SC_THR_CLASS: public Thread {
public:
	_SNE_SC_THR_CLASS(ServerNetworkEngine *ptr);
	void queue_sc(u16 player_id, u16 srv);
	void send(const SC_H data);
	void *run();
	bool _BEING_USED = false;
	
	//Mutex
	
	void lock();
	void unlock();
private:
	std::unordered_map<u16, SC_H> sc_s;
	ServerNetworkEngine *SNE = nullptr; //To initialize
	std::mutex mutex; //No race
};

/*
This will manage the files in the 'MineStars' folder on the world's path
*/
class _SNE_FILESYSTEM {
public:
	_SNE_FILESYSTEM() = default;
	void LOAD_FILESYSTEM(const std::string &path); //
	void SAVEFILE(const std::string &path, const std::string &raw);
	void LOADFILE(const std::string &path, std::ostringstream &data, bool &status);
	bool EXISTSFILE(const std::string &path);
	void CHECKFOLDER(const std::string &path);
	void REMOVEFILE(const std::string &path);
	//void _LOG(const std::string &str); //Save into a specific file for debugs
	std::string minestars_folder_path;
};

/*
THIS WILL ONLY GET EXTRA MEDIA FOUND IN SLAVES, NOT OVERRIDE MEDIA OF THE PROXY
IF A TEXTURE OF THE SLAVE ARE MODIFIED AND HAS A SAME NAME AS THE PROXY'S TEXTURE
WILL DON'T LOAD THE SLAVE TEXTURE AS PRIORITY
*/
template<typename _F>
class BoolField;

class _SNE_MEDIA {
	friend class _SNE_FILESYSTEM;
public:
	_SNE_MEDIA() = default;
	u32 getTotalMediaCount();
	bool loadMediaNcheckSums(u16 ID, NetworkPacket *pkt); //Slave has sent the data with sums, so we will check it to load
	void startLoadSlaveMedia(u16 ID, NetworkPacket *pkt); //This should send a 'get media' signal to the slave, to send us the media for loading
	//^ this also will override the meta config if any media gets modified, also overriding the media files
	std::unordered_map<std::string, MediaInfo> getMedia() { return media; }; //Should be used on getting checksums in Server
	void setSNEF(_SNE_FILESYSTEM *obj);
	NetworkPacket sendGetMedia(u16 ID);
	void setSNE(ServerNetworkEngine *SNE2) { SNE = SNE2; }
	void runMMS();
	void setMMMS(u16 _max_servers) { max_servers = _max_servers; }
	bool HasMedia(const std::string &name);
protected:
	void loadMeta(u16 max_count); //Only used on the SNE class, should be used on constructor of ServerNetworkEngine()
private:
	std::unordered_map<std::string, MediaInfo> media;
	std::unordered_map<u16, std::vector<std::string>> to_getmedia;
	std::unordered_map<u16, std::unordered_map<std::string, std::string>> sha1_storage;
	ServerNetworkEngine *SNE = nullptr;
	_SNE_FILESYSTEM *SNEF = nullptr;
	bool _CFG_SRVSTATE = false;
	BoolField<u16> *state_verif = nullptr;
	//BoolField<std::string> *media_verif = nullptr;
	//std::unordered_map<u16, BoolField<std::string>> media_verif;
	std::unordered_map<u16, std::unordered_map<std::string, bool>> to_delete;
	u16 max_servers = 0;
};

//Threading method
//Poor Mr. Thread, is being abused :(

//BlockDataThread <Helper in sending block data>


//Helper
template<typename T, typename T_>
class QuickThreading;

class PacketStreamThread: public Thread {
public:
	~PacketStreamThread() {};
	void *run();
	void queue(NetworkPacket pkt);
	PacketStreamThread(ServerNetworkEngine *SNE): SNE(SNE), Thread(std::string("PacketStreamThread")) {};
private:
	//bool has_enough_packets_to_continue();
	void doSpecialPacket(std::unordered_map<u16, NetworkPacket> o_packet, u16 last_pkt_o_packet);
	std::deque<NetworkPacket> stream;
	std::mutex mutex; //always use protection
	//std::unordered_map<u16, NetworkPacket> o_packet; //Here the packets are stored that needed a super order
	//u16 last_pkt_o_packet = 0;
	ServerNetworkEngine *SNE = nullptr;
	//QuickThreading<NetworkPacket, NetworkPacket> *_fish = nullptr;
	//Prefer a queue
	std::deque<QuickThreading<NetworkPacket, NetworkPacket>*> objects;
};

// Direct functions
struct LoaderFunctions {
	void (ServerNetworkEngine::*handler)(NetworkPacket* pkt);
};
extern const LoaderFunctions _Functions[MAX_FUNC_SCALE];

class ServerNetworkEngine_NetworkThread : public Thread {
	friend class ServerNetworkEngine;
public:
	virtual ~ServerNetworkEngine_NetworkThread();
	void *run();
protected:
	ServerNetworkEngine_NetworkThread(ServerNetworkEngine* jobDispatcher, u16 SID, const std::string &name, ActiveServersStructure *struct_) : jobDispatcher(jobDispatcher), ServerID(SID), ServerData(struct_), Thread(name) { IndependientIPaddress = ServerData->independient; pst_thr = new PacketStreamThread(jobDispatcher); pst_thr->start(); };
private:
	ServerNetworkEngine *jobDispatcher = nullptr;
	u16 ServerID = 0;
	ActiveServersStructure *ServerData = nullptr;
	bool IndependientIPaddress = false;
	PacketStreamThread *pst_thr = nullptr;
	bool SocketConnection = false; //If true, then there must be a dir to the socket file (Local Running Server)
	std::string SocketDirectory = "";
};

class SS_CREATOR;

// Will manage servers, this should be handled only by Server OBJ and the network thread
class ServerNetworkEngine: public _SNE_MEDIA, public _SNE_FILESYSTEM { //_SNE_FILESYSTEM needs to be public for slave_server_creator.h from HyperServer
	friend class ServerNetworkEngine_NetworkThread;
public:
	ServerNetworkEngine() = default;
	ServerNetworkEngine(ServerEnvironment *env, Server *Serv, const std::string &path);

	void SetEnv(ServerEnvironment *env) { m_env = env; }

	//Redirect packets to user defined server.
	//void RedirectPacket(NetworkPacket *pkt, ActiveServersStructure *Struct);
	void RedirectPacket(NetworkPacket *pkt, u16 ID_); //*

	void DoReCreateClient(u16 playerid);

	//Send disconnect to the player entity that stands on a slave server
	void SendDisconnect(u16 player_id, u16 ID); //TOSERVER_PLAYER_OUT
	void SendDisconnectONLY(u16 player_id, u16 ID);

	//Send connect to a server [Should wipe the player object of the main server] [Simulate player entrance with TOSERVER_PLAYER_ENTRANCE]
	void SendConnect(u16 player_id, u16 ID); //TOSERVER_PLAYER_ENTRANCE

	//Will receive some updates to apply into players [Server -> Proxy -> Player] [Data is parsed]
	void DoApplyEnvironmentToPlayer(NetworkPacket *pkt);

	u16 QueryThisServerID() { if (IsThisAProxy) { warningstream << "Requested QueryThisServerID() on proxy server" << std::endl; } return ss_ID; };
//Get this server ID

	bool isValidServer(u16 sid);

	void OnInitServer();
	void SetThisServerAsAproxy(bool b);
	void StopNkillThreads();

	void InitializeServer(u16 ID);

	void SetTSID(u16 ID) { ss_ID = ID; }

	//Registers a slave server and return his value
	u16 RegisterProxyServer(Address address, u16 ID_FOR_SERVER, bool IndependientBindAddress, u16 PortBindAddress);
	//Powers Down the entire network
	void PowerDownNetwork();
	bool initialized = false;
	bool IsThisAProxy = false; //Defined by default on mods.
	bool AreSlave = false;
	bool IndependientIPaddress = false;
	Address ProxyAddress;
	bool ProxyAddressAdded = false;
	std::unordered_map<u16, QueuedPacketsHelper*> PacketsQueue; //Used only if an independient address are not set (using proxy ip only)

	void SendRawToSlaveServer(u16 ID, NetworkPacket pkt);

	void sendQueryItemData(u16 ID);

	//Freeze/Unfreeze
	void FreezeServer();
	void unFreezeServer();

	Server *Server___ = nullptr;
	ServerEnvironment *m_env; //Shared env

	void setPlayerSAOidOfPlayerReverse(u16 SrvID, u16 ID, u16 ID_) {
		StoredServers[SrvID]->players_sao[ID] = ID_;
		StoredServers[SrvID]->reverse_players_sao[ID_] = ID;
	}
	void erasePlayerSAOid(u16 SrvID, u16 player_id) {
		StoredServers[SrvID]->players_sao.erase(StoredServers[SrvID]->reverse_players_sao[player_id]);
		StoredServers[SrvID]->reverse_players_sao.erase(player_id);
	}

	void SendWipeAllHuds(u16 pid);
	void QueueSendConnect(u16 player, u16 srv);
	void SendActivateBackground(u16 player);
	void SendWipeChat(u16 pid);
	void SendDeactivateBackground(u16 player);

	void _HELPER_SendConnect(u16 player_id, u16 ID); //Should not be used externally

	bool ServerHasMedia(const std::string &file);

	uint16_t getCountofServers() { return StoredServers.size(); }
	ActiveServersStructure *getAS(uint16_t ID) { return StoredServers.at(ID); }
	SS_CREATOR *SSC = nullptr;
	void PoweroffServers();
	void _handle_null(NetworkPacket *pkt);
	void _handle_direct(NetworkPacket *pkt);

	// Main functions for transform
	void handleDeleteParticleSpawner(NetworkPacket *pkt);
	void handleAddNode(NetworkPacket *pkt);
	void handleCaseDisconnect(NetworkPacket *pkt);
	void handleHello(NetworkPacket *pkt);
	void handleRemoveNode(NetworkPacket *pkt);
	void handleRemoveSAO(NetworkPacket *pkt);
	void handleUpdateList(NetworkPacket *pkt);
	void handleGotItemDef(NetworkPacket *pkt);
	void handleOnReceiveAOM(NetworkPacket *pkt);
	void handleGotMedia(NetworkPacket *pkt);
	void handleGotMedia_STAGETWO(NetworkPacket *pkt);
	void handleHandShake_STAGETWO(NetworkPacket *pkt);
	void handlePlayerInitialization(NetworkPacket *pkt);
	void handleTimeOfDay(NetworkPacket *pkt);
	void handleChatMessage(NetworkPacket *pkt);
	void handleHP(NetworkPacket *pkt);
	void handlePlaySound(NetworkPacket *pkt);
	void handleStopSound(NetworkPacket *pkt);
	void handleFadeSound(NetworkPacket *pkt);
	void handleDetachedInventory(NetworkPacket *pkt);
	void handleNodemetaChanged(NetworkPacket *pkt);
	void handleAO_RA(NetworkPacket *pkt);
private:
	void CreateNewThreadStructureForServer(ActiveServersStructure *struct_);

	std::unordered_map<u16, u16> PlayerSAOtoPlayerIDs;
	std::unordered_map<u16, u16> PlayerSaoIDs;
	std::unordered_map<u16, ActiveServersStructure*> StoredServers;
	std::vector<ServerNetworkEngine_NetworkThread*> threads;
	std::unordered_map<u16, bool> hasDeletedSAO; //Used for checks
	SlaveItemDef *Slave_ItemDef = nullptr;
	u16 ss_ID = 0;
	std::unordered_map<u16, std::tuple<u32, u32>> fun_huds; //idk how to name it
	//DA
	_SNE_SC_THR_CLASS *_SNE_SC_THR_OBJ = nullptr;
};



















