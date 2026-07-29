/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
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

#include <cstdint>
#include <unordered_map>
#include <algorithm>
#include <vector>

#include "Iserver.hpp"
#include "irr_v3d.h"
#include "map.h"
#include "hud.h"
#include "gamedef.h"
#include "serialization.h" // For SER_FMT_VER_INVALID
#include "content/mods.h"
#include "inventorymanager.h"
#include "content/subgames.h"
#include "tileanimation.h" // TileAnimationParams
#include "particles.h" // ParticleParams
#include "network/peerhandler.h"
#include "network/address.h"
#include "util/numeric.h"
#include "util/thread.h"
#include "util/basic_macros.h"
#include "util/metricsbackend.h"
#include "serverenvironment.h"
#include "constants.h"
#include "clientiface.h"
#include "chatmessage.h"
#include "translation.h"
#include "media.h" // MediaInfo
#include <string>
#include <list>
#include <map>
#include <vector>
#include "core/lib/mt_map.h"
//#include "core/lib/mt_queue.h"

extern uint16_t ThisServID;

class ChatEvent;
struct ChatEventChat;
struct ChatInterface;
class IWritableItemDefManager;
class NodeDefManager;
class IWritableCraftDefManager;
class BanManager;
class EventManager;
class Inventory;
class RemotePlayer;
class PlayerSAO;
struct PlayerHPChangeReason;
//class IRollbackManager;
//struct RollbackAction;
class EmergeManager;
class ServerScripting;
class ServerEnvironment;
struct SimpleSoundSpec;
struct CloudParams;
struct SkyboxParams;
struct SunParams;
struct MoonParams;
struct StarParams;
class ServerThread;
class NetworkThread;
class ServerModManager;
class ServerInventoryManager;
struct PackedValue;
class ClientDataHelper;
class ServerNetworkEngine;
class NetworkCompressor;
class SentNetworkThread;
class ServSectorScript;
class ServSectorEnv;
class ThreadingAOM;
class ServerMapFilesSaver;
class MapThread;
class ServInternal;
class MapUpdate;


struct PlayerInternalInfo {
	PlayerInternalInfo() = default;
	uint16_t PlayerID;
	uint16_t ServerID;
	session_t PeerID;
	bool PlayingOnServ;
};

typedef uint16_t MAP_ID;

struct PlayerDataOMM {
	uint16_t mapid; //Map which the player are standing rn
	std::unordered_map<MAP_ID, v3f> MAP;
};

struct HyperMap {
	HyperMap() = default;
	ServerMap *m_map = nullptr;
	uint16_t m_id = 0;
	std::string m_name;
	//server::ActiveObjectMgr m_ao_mgr; //Not easy to maintain. So. i've decided to make it shared along all maps (May overflow)
	EmergeManager *m_emerge = nullptr;
	MapThread *m_thread = nullptr;
};

//useful for queues
struct PlayerInitializer {
	PlayerInitializer() = default;
	session_t pid;
};

struct PlayerInitializerSLV {
	PlayerInitializerSLV() = default;
	PlayerSAO *SAO;
	const char *NAME;
};

struct ServerPlayingSound
{
	ServerSoundParams params;
	SimpleSoundSpec spec;
	std::unordered_set<session_t> clients; // peer ids
	std::unordered_set<uint16_t> clients_int16; // same as the variable up this line but with uint16_t
};

class servAddons;

class Server : public IServer
{
public:
	/*
		NOTE: Every public method should be thread-safe
	*/

	Server(const std::string &path, const SubgameSpec &GS, bool ssm, Address addr, bool d);
	~Server();
	DISABLE_CLASS_COPY(Server);

	void start();
	void stop();
	// This is mainly a way to pass the time to the server.
	// Actual processing is done in an another thread.
	void step(float dtime);
	// This is run by ServerThread and does the actual processing
	void Receive();
	PlayerSAO* StageTwoClientInit(uint16_t _ID);

	//Server Network Object
	ServerNetworkEngine *ServersNetworkObject = nullptr;
	//con::Connection *MAINSERVEURE = nullptr;
	std::shared_ptr<con::Connection> MAINSERVEURE;

	std::string SocketDir = "";
	bool SocketConn = false;
	int SocketID = 0;
	servAddons *sAddons = nullptr;

	/*
	 * Command Handlers
	 */

	void handleCommand(NetworkPacket* pkt);

	void handleCommand_Null(NetworkPacket* pkt) {};
	void handleCommand_Deprecated(NetworkPacket* pkt);
	void handleCommand_Init(NetworkPacket* pkt);
	void handleCommand_Init2(NetworkPacket* pkt);
	void handleCommand_RequestMedia(NetworkPacket* pkt);
	void handleCommand_ClientReady(NetworkPacket* pkt);
	void handleCommand_GotBlocks(NetworkPacket* pkt);
	void handleCommand_PlayerPos(NetworkPacket* pkt);
	void handleCommand_DeletedBlocks(NetworkPacket* pkt);
	void handleCommand_InventoryAction(NetworkPacket* pkt);
	void handleCommand_ChatMessage(NetworkPacket* pkt);
	void handleCommand_Damage(NetworkPacket* pkt);
	void handleCommand_PlayerItem(NetworkPacket* pkt);
	void handleCommand_Respawn(NetworkPacket* pkt);
	void handleCommand_Interact(NetworkPacket* pkt);
	void handleCommand_RemovedSounds(NetworkPacket* pkt);
	void handleCommand_NodeMetaFields(NetworkPacket* pkt);
	void handleCommand_InventoryFields(NetworkPacket* pkt);
	void handleCommand_FirstSrp(NetworkPacket* pkt);
	void handleCommand_SrpBytesA(NetworkPacket* pkt);
	void handleCommand_SrpBytesM(NetworkPacket* pkt);
	void handleCommand_GotConnect(NetworkPacket* pkt);
	void handleCommand_GotDisconnect(NetworkPacket* pkt);
	void handleCommand_JumpDefinitions(NetworkPacket *pkt);

	void ProcessData(NetworkPacket *pkt);

	void Send(NetworkPacket *pkt);
	void Send(NetworkPacket *pkt, bool relative);
	void Send(session_t peer_id, NetworkPacket *pkt);
	void Send(session_t peer_id, NetworkPacket *pkt, bool relative);

	void HandleProxyCommand(NetworkPacket *pkt);
	void OnlyDeleteSAO(session_t peer_id, uint16_t ID_);


	std::mutex DEFINITIONS_EXECUTION;

	std::unordered_map<u16, u16> Jumper; //Only for slave
	void SendActiveObjectMessages(session_t peer_id, const std::string &datas, bool reliable = true);

	// Helper for handleCommand_PlayerPos and handleCommand_Interact
	void process_PlayerPos(RemotePlayer *player, PlayerSAO *playersao,
		NetworkPacket *pkt);

	// Both setter and getter need no envlock,
	// can be called freely from threads
	void setTimeOfDay(u32 time);

	/*
		Shall be called with the environment locked.
		This is accessed by the map, which is inside the environment,
		so it shouldn't be a problem.
	*/
	void onMapEditEvent(const MapEditEvent &event);

	// Connection must be locked when called
	std::string getStatusString();
	inline double getUptime() const { return m_uptime_counter->get(); }

	// read shutdown state
	inline bool isShutdownRequested() const { return m_shutdown_state.is_requested; }

	// request server to shutdown
	void requestShutdown(const std::string &msg, bool reconnect, float delay = 0.0f);

	// Returns -1 if failed, sound handle on success
	// Envlock
	int32_t playSound(const SimpleSoundSpec &spec, const ServerSoundParams &params,
			bool ephemeral=false);
	void stopSound(int32_t handle);
	void fadeSound(int32_t handle, float step, float gain);

	// Envlock
	std::set<std::string> getPlayerEffectivePrivs(const std::string &name);
	bool checkPriv(const std::string &name, const std::string &priv);
	void reportPrivsModified(const std::string &name=""); // ""=all
	void reportInventoryFormspecModified(const std::string &name);
	void reportFormspecPrependModified(const std::string &name);

	void setIpBanned(const std::string &ip, const std::string &name);
	void unsetIpBanned(const std::string &ip_or_name);
	std::string getBanDescription(const std::string &ip_or_name);

	void notifyPlayer(const char *name, const std::wstring &msg);
	void notifyPlayers(const std::wstring &msg);

	void spawnParticle(const std::string &playername,
		const ParticleParameters &p);

	uint32_t addParticleSpawner(const ParticleSpawnerParameters &p,
		ServerActiveObject *attached, const std::string &playername);

	void deleteParticleSpawner(const std::string &playername, u32 id);

	bool dynamicAddMedia(const std::string &filepath, std::vector<RemotePlayer*> &sent_to);
	bool addMediaFile(const std::string &filename, const std::string &filepath,
			std::string *filedata = nullptr, std::string *digest = nullptr);

	ServerInventoryManager *getInventoryMgr() const { return m_inventory_mgr.get(); }
	void sendDetachedInventory(Inventory *inventory, const std::string &name, session_t peer_id);

	// Envlock and conlock should be locked when using scriptapi
	ServerScripting *getScriptIface(){ return m_script; }

	// IGameDef interface
	// Under envlock
	virtual IItemDefManager* getItemDefManager();
	virtual const NodeDefManager* getNodeDefManager();
	virtual ICraftDefManager* getCraftDefManager();
	virtual u16 allocateUnknownNodeId(const std::string &name);
	//IRollbackManager *getRollbackManager() { return m_rollback; }
	virtual EmergeManager *getEmergeManager(uint16_t mapid = 0) { return Maps.at(mapid)->m_emerge; }

	IWritableItemDefManager* getWritableItemDefManager();
	NodeDefManager* getWritableNodeDefManager();
	IWritableCraftDefManager* getWritableCraftDefManager();

	//virtual const std::vector<ModSpec> &getMods() const;
	//virtual const ModSpec* getModSpec(const std::string &modname) const;
	static std::string getBuiltinLuaPath();
	virtual std::string getWorldPath() const { return m_path_world; }
	//virtual std::string getModStoragePath() const;

	inline bool isSingleplayer()
			{ return false; }

	inline void setAsyncFatalError(const std::string &error)
			{ m_async_fatal_error.set(error); }

	bool showFormspec(const char *name, const std::string &formspec, const std::string &formname);
	Map & getMap();
	ServerEnvironment & getEnv() { return *m_env; }
	v3f findSpawnPos(uint16_t map_id = 0);

	uint32_t hudAdd(RemotePlayer *player, HudElement *element);
	bool hudRemove(RemotePlayer *player, uint32_t id);
	bool hudChange(RemotePlayer *player, u32 id, HudElementStat stat, void *value);
	bool hudSetFlags(RemotePlayer *player, u32 flags, u32 mask);
	bool hudSetHotbarItemcount(RemotePlayer *player, s32 hotbar_itemcount);
	void hudSetHotbarImage(RemotePlayer *player, const std::string &name);
	void hudSetHotbarSelectedImage(RemotePlayer *player, const std::string &name);

	Address getPeerAddress(session_t peer_id);

	void setLocalPlayerAnimations(RemotePlayer *player, v2s32 animation_frames[4],
			f32 frame_speed);
	void setPlayerEyeOffset(RemotePlayer *player, const v3f &first, const v3f &third);

	void setSky(RemotePlayer *player, const SkyboxParams &params);
	void setSun(RemotePlayer *player, const SunParams &params);
	void setMoon(RemotePlayer *player, const MoonParams &params);
	void setStars(RemotePlayer *player, const StarParams &params);

	void setClouds(RemotePlayer *player, const CloudParams &params);

	void overrideDayNightRatio(RemotePlayer *player, bool do_override, float brightness);

	/* con::PeerHandler implementation. */
	void peerAdded(con::Peer *peer);
	void deletingPeer(con::Peer *peer, bool timeout);

	void DenySudoAccess(uint16_t peer_id);
	void DenyAccess(uint16_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason = "", bool reconnect = false);
	void acceptAuth(uint16_t peer_id, bool forSudoMode);
	void DisconnectPeer(uint16_t peer_id);
	bool getClientConInfo(uint16_t peer_id, con::rtt_stat_type type, float *retval);
	bool getClientInfo(uint16_t peer_id, ClientInfo &ret);

	void printToConsoleOnly(const std::string &text);

	void SendPlayerHPOrDie(PlayerSAO *player, const PlayerHPChangeReason &reason);
	void SendPlayerBreath(PlayerSAO *sao);
	void SendInventory(PlayerSAO *playerSAO, bool incremental);
	void SendMovePlayer(uint16_t peer_id, v3f overridepos = {72000,0,0});
	void SendPlayerSpeed(uint16_t peer_id, const v3f &added_vel);
	void SendPlayerFov(uint16_t peer_id);

	void SendMinimapModes(uint16_t peer_id, std::vector<MinimapMode> &modes, size_t wanted_mode);

	void sendDetachedInventories(uint16_t peer_id, bool incremental);

	//virtual bool registerModStorage(ModMetadata *storage);
	//virtual void unregisterModStorage(const std::string &name);

	// Send block to specific player only
	bool SendBlock(uint16_t peer_id, const v3s16 &blockpos);

	// Get or load translations for a language
	Translations *getTranslationLanguage(const std::string &lang_code);

	// Lua files registered for init of async env, pair of modname + path
	std::vector<std::pair<std::string, std::string>> m_async_init_files;
	// Identical but for mapgen env
	std::vector<std::pair<std::string, std::string>> m_mapgen_init_files;

	// Serialized data transferred into async envs at init time
	MutexedVariable<std::string> m_async_globals_data;

	// Data transferred into other Lua envs at init time
	//std::unique_ptr<PackedValue> m_lua_globals_data;

	//Some data that will be used for the slave
	//std::map<u16, std::map<v3s16, float>> SlaveClientMap;
	//std::map<u16, float> m_nothing_to_send_pause_timerT;
	//std::map<>
	

	// Bind address
	Address m_bind_addr;

	// Environment mutex (envlock)
	std::mutex m_env_mutex;

	inline bool isCompatPlayerModel(const std::string &model_name)
	{
		return std::find(m_compat_player_models.begin(), m_compat_player_models.end(), model_name) != m_compat_player_models.end();
	}
	const std::vector<std::string> getCompatPlayerModels()
	{
		return m_compat_player_models;
	}
	
	//Moved to public for SNE
	std::unordered_map<std::string, MediaInfo> m_media;

	SubgameSpec getGameSpec() { return m_gamespec; }

	std::mutex PacketsDequeMTX;
	std::deque<NetworkPacket> PacketsDeque;

	SentNetworkThread *SNT = nullptr;
	// Environment
	ServerEnvironment *m_env = nullptr;

	ServSectorEnv *SSE = nullptr;
	ServSectorScript *SSS = nullptr;
	ThreadingAOM *TAOM = nullptr;
	/*
	 Q ueue of map edits from the environment for sending to *the clients
	 This is behind m_env_mutex
	 */
	MultithreadQueue<MapEditEvent*> m_unsent_map_edit_queue;
	MultithreadQueue<NetworkPacket> QueuedPackets; //This is needed to handle packets for the main server (Specified as main proxy players at ONLY.)
	bool setPlayerOnMap(RemotePlayer *player, uint16_t mapid, v3f def_pos = {0,0,0});
	bool unSetPlayerOnMap(RemotePlayer *player);
	std::list<RemotePlayer*> getPlayersInMap(uint16_t mapid);
	//Players must be saved in a file to know which world they was and their positions
	ServerMapFilesSaver *m_smfs = nullptr;
	void loadMapFiles(); //map.cpp
	void saveMapFiles(); //map.cpp
	bool createNewMap(const std::string mapname, uint16_t *mapid); //map.cpp
	bool deleteMap(uint16_t mapid); //map.cpp
	void AsyncRunStepSlave(bool initial_step); //init.cppAsyncRunStepAsyncRunStep
	void AsyncRunStepMain(bool initial_step); //init.cpp
	MultithreadMap<uint16_t, MultithreadMap<uint16_t, RemotePlayer*>*> MapToPlayers; //*
	MultithreadMap<std::string, PlayerDataOMM> PlayerToMap; //*
	void SendRemoveObjectsToClient(u16 pid, bool keep_attached_ones);
	void QueueProcessData(NetworkPacket pkt);
	void DoSendToEveryone(NetworkPacket *pkt, NetworkPacket *legacy_pkt, u16 SID);
	void sendUpdatePlayerSaoList(u16 playerid, std::unordered_map<u16, u16> playerlistNsao);
	void SendDisconnectToPlayer(u16 ID);
	void DeleteClient(session_t peer_id, ClientDeletionReason reason);
	void handleCommand_PlayerPosAdvanced(NetworkPacket *pkt);
	void sendRemoveNode(v3s16 p, std::unordered_set<u16> *far_players = nullptr,float far_d_nodes = 100);
	void sendAddNode(v3s16 p, MapNode n, std::unordered_set<u16> *far_players = nullptr, float far_d_nodes = 100, bool remove_metadata = true);
	// server connection; Sorry. . .
	std::shared_ptr<con::Connection> m_con;
	ClientInterface m_clients;
	void DisconnectAllPlayers(bool crashed = false);
	void sendDefinitions();
	u16 GetRandomIDforPlayer();
	void QueuePlayerToInitialize(session_t p);
	bool ExistsID(u16 ID);
	void QueueOnJoinPlayer(PlayerSAO *sao, const char *name);
	void HandleIDforServer(NetworkPacket *pkt);
	void MakeThisASlaveServer(u16 id);
	bool equalsPlayerIDwithPeerID(u16 PlayerID, session_t PeerID);
	float getRecommendedSendInterval();
	u16 getProtocolOfThisPeer(session_t PeerID);
	void QueueSendTo(session_t peer, NetworkPacket pkt, bool reliable);
	RemoteClient* getClient(uint16_t peer_id, ClientState state_min = CS_Active);
	MultithreadMap<uint16_t, ClientDataHelper*> ClientDataTable; //init.cpp::slave
	void sendMetadataChanged(const std::list<v3s16> &meta_updates, float far_d_nodes = 100);
	PlayerSAO *getPlayerSAO(uint16_t peer_id);
	std::string m_path_world;
	PlayerSAO *InitClientByMainServer(u16 ID, ClientDataHelper *client);
	void DeletePlayer(u16 p);
	uint8_t getSerializationVersion(session_t peer_id);
	std::vector<bool> EnabledPlayers; // Once the first player packet is checked without problems we can't check it again
	MultithreadMap<uint16_t, PlayerInternalInfo> Players;
	MultithreadMap<session_t, PlayerInternalInfo*> PeerIdPlayers;
	ClientDataHelper *getClientCDH(uint16_t p_id);
	bool existsMap(uint16_t mapid) { return Maps.find(mapid) != Maps.end(); }
	/* mark blocks not sent for all clients */
	void SetBlocksNotSent(std::map<v3s16, MapBlock *>& block);
private:
	friend class EmergeThread;
	friend class RemoteClient;
	friend class TestServerShutdownState;
	friend class NetworkThread;

	struct ShutdownState {
		friend class TestServerShutdownState;
		public:
			bool is_requested = false;
			bool should_reconnect = false;
			std::string message;

			void reset();
			void trigger(float delay, const std::string &msg, bool reconnect);
			void tick(float dtime, Server *server);
			std::wstring getShutdownTimerMessage() const;
			bool isTimerRunning() const { return m_timer > 0.0f; }
		private:
			float m_timer = 0.0f;
	};

	void init();

	void SendMovement(uint16_t peer_id, uint16_t protocol_version);
	void SendHP(uint16_t peer_id, uint16_t hp);
	void SendBreath(uint16_t peer_id, uint16_t breath);
	void SendAccessDenied(uint16_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason, bool reconnect = false);
	void SendAccessDenied_Legacy(uint16_t peer_id, const std::wstring &reason);
	void SendDeathscreen(uint16_t peer_id, bool set_camera_point_target,
		v3f camera_point_target);
	void SendItemDef(uint16_t peer_id, IItemDefManager *itemdef, u16 protocol_version);
	void SendNodeDef(uint16_t peer_id, const NodeDefManager *nodedef,
		u16 protocol_version);

	


	virtual void SendChatMessage(uint16_t peer_id, const ChatMessage &message);
	void SendTimeOfDay(uint16_t peer_id, u16 time, f32 time_speed);
	void SendPlayerHP(uint16_t peer_id);

	void SendLocalPlayerAnimations(uint16_t peer_id, v2s32 animation_frames[4], f32 animation_speed);
	void SendEyeOffset(uint16_t peer_id, v3f first, v3f third);
	void SendPlayerPrivileges(uint16_t peer_id);
	void SendPlayerInventoryFormspec(uint16_t peer_id);
	void SendPlayerFormspecPrepend(uint16_t peer_id);
	void SendShowFormspecMessage(uint16_t peer_id, const std::string &formspec, const std::string &formname);
	void SendHUDAdd(uint16_t peer_id, u32 id, HudElement *form);
	void SendHUDRemove(uint16_t peer_id, u32 id);
	void SendHUDChange(uint16_t peer_id, u32 id, HudElementStat stat, void *value);
	void SendHUDSetFlags(uint16_t peer_id, u32 flags, u32 mask);
	void SendHUDSetParam(uint16_t peer_id, u16 param, const std::string &value);
	void SendSetSky(uint16_t peer_id, const SkyboxParams &params);
	void SendSetSun(uint16_t peer_id, const SunParams &params);
	void SendSetMoon(uint16_t peer_id, const MoonParams &params);
	void SendSetStars(uint16_t peer_id, const StarParams &params);
	void SendCloudParams(uint16_t peer_id, const CloudParams &params);
	void SendOverrideDayNightRatio(uint16_t peer_id, bool do_override, float ratio);

	// Environment and Connection must be locked when called
	void SendBlockNoLock(uint16_t peer_id, MapBlock *block, u8 ver, u16 net_proto_version);

	// Sends blocks to clients (locks env and con on its own)
	void SendBlocks(float dtime);

	void fillMediaCache();
	void sendMediaAnnouncement(uint16_t peer_id, const std::string &lang_code);
	void sendRequestedMedia(uint16_t peer_id,
			const std::vector<std::string> &tosend);

	// Adds a ParticleSpawner on peer with peer_id (PEER_ID_INEXISTENT == all)
	void SendAddParticleSpawner(uint16_t peer_id, u16 protocol_version,
		const ParticleSpawnerParameters &p, u16 attached_id, u32 id);

	void SendDeleteParticleSpawner(uint16_t peer_id, u32 id);

	//Send data to main server, if this is an slave
	void SendToMainServer(NetworkPacket *pkt);

	// Spawns particle on peer with peer_id (PEER_ID_INEXISTENT == all)
	void SendSpawnParticle(uint16_t peer_id, u16 protocol_version,
		const ParticleParameters &p);

	void SendActiveObjectRemoveAdd(RemoteClient *client, PlayerSAO *playersao);
	void SendCSMRestrictionFlags(session_t peer_id);

	void SendActiveObjectRemoveAdd_SLAVE(ClientDataHelper *client, PlayerSAO *playersao);

	void InitializePlayer(session_t pid);

	/*
		Something random
	*/

	void DiePlayer(uint16_t peer_id, PlayerHPChangeReason &reason);
	void RespawnPlayer(uint16_t peer_id);
	void UpdateCrafting(RemotePlayer *player);
	bool checkInteractDistance(RemotePlayer *player, const f32 d, const std::string &what);

	void handleChatInterfaceEvent(ChatEvent *evt);

	// This returns the answer to the sender of wmessage, or "" if there is none
	std::wstring handleChat(const std::string &name, std::wstring wmessage_input,
		bool check_shout_priv = false, RemotePlayer *player = nullptr);
	void handleAdminChat(const ChatEventChat *evt);

	// When called, connection mutex should be locked

	RemoteClient* getClientNoEx(uint16_t peer_id, ClientState state_min = CS_Active);

	// When called, environment mutex should be locked
	std::string getPlayerName(uint16_t peer_id);

	/*
		Get a player from memory or creates one.
		If player is already connected, return NULL
		Does not verify/modify auth info and password.

		Call with env and con locked.
	*/
	PlayerSAO *emergePlayer(const char *name, uint16_t peer_id, u16 proto_version);

	void handlePeerChanges();

	/*
		Variables
	*/
	// Subgame specification
	SubgameSpec m_gamespec;
	// If true, do not allow multiple players and hide some multiplayer
	// functionality
	bool m_simple_singleplayer_mode;
	u16 m_max_chatmessage_length;
	// For "dedicated" server list flag
	bool m_dedicated;
	Settings *m_game_settings = nullptr;

	// Thread can set; step() will throw as ServerError
	MutexedVariable<std::string> m_async_fatal_error;

	// Some timers
	float m_liquid_transform_timer = 0.0f;
	float m_liquid_transform_every = 1.0f;
	float m_masterserver_timer = 0.0f;
	float m_emergethread_trigger_timer = 0.0f;
	float m_savemap_timer = 0.0f;
	IntervalLimiter m_map_timer_and_unload_interval;



	// Reference to the server map until ServerEnvironment is initialized
	// after that this variable must be a nullptr
	ServerMap *m_startup_server_map = nullptr;

	// Ban checking
	BanManager *m_banmanager = nullptr;

	// Rollback manager (behind m_env_mutex)
	//IRollbackManager *m_rollback = nullptr;

	// Emerge manager
	EmergeManager *m_emerge = nullptr; //Only for map 0

	// Scripting
	// Envlock and conlock should be locked when using Lua
	ServerScripting *m_script = nullptr;

	// Item definition manager
	IWritableItemDefManager *m_itemdef;

	// Node definition manager
	NodeDefManager *m_nodedef;

	// Craft definition manager
	IWritableCraftDefManager *m_craftdef;

	// Mods
	std::unique_ptr<ServerModManager> m_modmgr;

	std::unordered_map<std::string, Translations> server_translations;

	/*
		Threads
	*/
	// A buffer for time steps
	// step() increments and AsyncRunStep() run by m_thread reads it.
	float m_step_dtime = 0.0f;
	std::mutex m_step_dtime_mutex;

	// The server mainly operates in this thread
	ServInternal *m_thread = nullptr;
	MapUpdate *m_mapupdate = nullptr;
	NetworkThread *m_netthr = nullptr;

	/*
		Time related stuff
	*/
	// Timer for sending time of day over network
	float m_time_of_day_send_timer = 0.0f;

	/*
	 	Client interface
	*/

	//ClientInterface16 m_clients16;



	//Some tables that might help to make multithreaded context
	//std::mutex QueuedPacketsMTX;
	//std::unordered_map<session_t, u16> SeenSlaveServersIDS; //NOTE: UNSUPPORTED

	std::unordered_map<uint16_t, std::unordered_map<uint16_t, PlayerInternalInfo*>> Servers;

	/*
		Peer change queue.
		Queues stuff from peerAdded() and deletingPeer() to
		handlePeerChanges()
	*/
	std::queue<con::PeerChange> m_peer_change_queue;

	std::unordered_map<uint16_t, std::string> m_formspec_state_data;
	
	std::unordered_map<u16, std::string> m_formspec_state_dataU16;

	/*
		Random stuff
	*/

	ShutdownState m_shutdown_state;

	ChatInterface *m_admin_chat;
	std::string m_admin_nick;

	// if a mod-error occurs in the on_shutdown callback, the error message will
	// be written into this
	//std::string *const m_on_shutdown_errmsg;

	/*
		Map edit event queue. Automatically receives all map edits.
		The constructor of this class registers us to receive them through
		onMapEditEvent

		NOTE: Should these be moved to actually be members of
		ServerEnvironment?
	*/


	/*
		If a non-empty area, map edit events contained within are left
		unsent. Done at map generation time to speed up editing of the
		generated area, as it will be sent anyway.
		This is behind m_env_mutex
	*/
	VoxelArea m_ignore_map_edit_events_area;

	// media files known to server
	
	std::unordered_map<std::string, InMemoryMediaInfo> m_compat_media;

	/*
		Sounds
	*/
	std::unordered_map<int32_t, ServerPlayingSound> m_playing_sounds;
	int32_t m_next_sound_id = 0; // positive values only
	int32_t nextSoundId();

	std::unordered_map<std::string, ModMetadata *> m_mod_storages;
	float m_mod_storage_save_timer = 10.0f;

	// CSM restrictions byteflag
	u64 m_csm_restriction_flags = CSMRestrictionFlags::CSM_RF_NONE;
	u32 m_csm_restriction_noderange = 8;

	// Inventory manager
	std::unique_ptr<ServerInventoryManager> m_inventory_mgr;

	// Global server metrics backend
	std::unique_ptr<MetricsBackend> m_metrics_backend;

	// Server metrics
	MetricCounterPtr m_uptime_counter;
	MetricGaugePtr m_player_gauge;
	MetricGaugePtr m_timeofday_gauge;
	// current server step lag
	MetricGaugePtr m_lag_gauge;
	MetricCounterPtr m_aom_buffer_counter;
	MetricCounterPtr m_packet_recv_counter;
	MetricCounterPtr m_packet_recv_processed_counter;

	std::vector<std::string> m_compat_player_models;
	
	std::mutex PlayersToInitialize_MTX;
	std::deque<session_t> PlayersToInitialize;
	

	
	std::mutex QueuedPlayersToInitialize;
	std::deque<PlayerInitializerSLV> ToInitialize;
	
	bool m_definitions_done = false;
	NetworkCompressor *NetCR = nullptr;
	//std::unordered_map<u16, std::unordered_map<>> //as table
	//std::unordered_map<u16, uint16_t> ActivePlayersOnProxy;
	bool *AreSlave = nullptr;

	//Multi map environment

	std::unordered_map<uint16_t, HyperMap*> Maps;
	std::vector<uint16_t> SessionToPlayer;
	bool m_powering_down = false;
};

/*
	Runs a simple dedicated server loop.

	Shuts down when kill is set to true.
*/
void dedicated_server_loop(Server &server, bool &kill);
