/*
	IServer.h

	Pure virtual interface extracted from the public surface of `Server`
	(server.h). Intended as the stable contract crossing a .so <-> host
	executable boundary:

	  - The .so implements a class, e.g. `class Server : public IServer { ... }`
	    (Server already exists with these signatures, so it satisfies this
	    interface with zero changes to its method bodies.)
	  - The host executable only ever sees `IServer*`, obtained via a C-linkage
	    factory function (see the bottom of this file), and calls virtual
	    methods through the vtable.

	NOTES / THINGS INTENTIONALLY LEFT OUT (see explanation below the class):
	  - The constructor, destructor body, and DISABLE_CLASS_COPY(Server) are
	    not part of the interface (constructors can't be virtual; copy is
	    disabled at the concrete-class level, not the interface level).
	  - Public *data members* (m_env, m_clients, Maps, etc.) are NOT included.
	    A vtable-based interface can only expose behavior (methods), not
	    storage layout -- struct/member layout is not ABI-stable across
	    compilers/versions the way a pure-virtual method table is. If the
	    host needs access to that state, add accessor methods instead.
	  - `getBuiltinLuaPath()` is `static` in the original class. Static
	    methods don't participate in the vtable, so it's kept as a static
	    (non-virtual) method here -- that's legal in an abstract class.
	  - Inline one-liner bodies (e.g. `isSingleplayer() { return false; }`)
	    are declared here without bodies; the concrete class still defines
	    them (Server already does).
*/

#pragma once

// NOTE: this interface header needs the same type definitions as the
// original server.h, since many methods pass these types by value/reference
// (not just as pointers). Include the same dependency headers here:
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <string>
#include <list>
#include <map>
#include <deque>
#include <set>

#include "irr_v3d.h"
#include "map.h"
#include "hud.h"
#include "gamedef.h"
#include "serialization.h"
#include "content/mods.h"
#include "inventorymanager.h"
#include "content/subgames.h"
#include "tileanimation.h"
#include "particles.h"
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
#include "media.h"
#include "itemdef.h"
#include "craftdef.h"

// Forward declarations (same as server.h)
class ChatEvent;
struct ChatEventChat;
class RemotePlayer;
class PlayerSAO;
struct PlayerHPChangeReason;
class EmergeManager;
class ServerScripting;
class ServerEnvironment;
struct SimpleSoundSpec;
struct CloudParams;
struct SkyboxParams;
struct SunParams;
struct MoonParams;
struct StarParams;
class ServerModManager;
class ServerInventoryManager;
class ClientDataHelper;
class ServerActiveObject;

// Types reused from server.h that the interface's method signatures depend on
enum ClientDeletionReason {
	CDR_LEAVE,
	CDR_TIMEOUT,
	CDR_DENY
};

struct MinimapMode {
	MinimapType type = MINIMAP_TYPE_OFF;
	std::string label;
	uint16_t size = 0;
	std::string texture;
	uint16_t scale = 1;
};

struct ServerSoundParams
{
    enum Type {
        SSP_LOCAL,
        SSP_POSITIONAL,
        SSP_OBJECT
    } type = SSP_LOCAL;
    float gain = 1.0f;
    float fade = 0.0f;
    float pitch = 1.0f;
    bool loop = false;
    float max_hear_distance = 32 * BS;
    v3f pos;
    uint16_t object = 0;
    std::string to_player = "";
    std::string exclude_player = "";
    
    v3f getPos(ServerEnvironment *env, bool *pos_exists) const;
};

struct ClientInfo {
	ClientState state;
	Address addr;
	u32 uptime;
	u8 ser_vers;
	uint16_t prot_vers;
	u8 major, minor, patch;
	std::string vers_string, platform, sysinfo, lang_code;
};

class IServer : public con::PeerHandler, public MapEventReceiver, public IGameDef
{
public:
	virtual ~IServer() = default;

	/*
		Core loop
	*/
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void step(float dtime) = 0;
	virtual void Receive() = 0;
	virtual PlayerSAO* StageTwoClientInit(uint16_t _ID) = 0;

	/*
		Command handlers
	*/
	virtual void handleCommand(NetworkPacket* pkt) = 0;
	virtual void handleCommand_Null(NetworkPacket* pkt) = 0;
	virtual void handleCommand_Deprecated(NetworkPacket* pkt) = 0;
	virtual void handleCommand_Init(NetworkPacket* pkt) = 0;
	virtual void handleCommand_Init2(NetworkPacket* pkt) = 0;
	virtual void handleCommand_RequestMedia(NetworkPacket* pkt) = 0;
	virtual void handleCommand_ClientReady(NetworkPacket* pkt) = 0;
	virtual void handleCommand_GotBlocks(NetworkPacket* pkt) = 0;
	virtual void handleCommand_PlayerPos(NetworkPacket* pkt) = 0;
	virtual void handleCommand_DeletedBlocks(NetworkPacket* pkt) = 0;
	virtual void handleCommand_InventoryAction(NetworkPacket* pkt) = 0;
	virtual void handleCommand_ChatMessage(NetworkPacket* pkt) = 0;
	virtual void handleCommand_Damage(NetworkPacket* pkt) = 0;
	virtual void handleCommand_PlayerItem(NetworkPacket* pkt) = 0;
	virtual void handleCommand_Respawn(NetworkPacket* pkt) = 0;
	virtual void handleCommand_Interact(NetworkPacket* pkt) = 0;
	virtual void handleCommand_RemovedSounds(NetworkPacket* pkt) = 0;
	virtual void handleCommand_NodeMetaFields(NetworkPacket* pkt) = 0;
	virtual void handleCommand_InventoryFields(NetworkPacket* pkt) = 0;
	virtual void handleCommand_FirstSrp(NetworkPacket* pkt) = 0;
	virtual void handleCommand_SrpBytesA(NetworkPacket* pkt) = 0;
	virtual void handleCommand_SrpBytesM(NetworkPacket* pkt) = 0;
	virtual void handleCommand_GotConnect(NetworkPacket* pkt) = 0;
	virtual void handleCommand_GotDisconnect(NetworkPacket* pkt) = 0;
	virtual void handleCommand_JumpDefinitions(NetworkPacket *pkt) = 0;
	virtual void handleCommand_PlayerPosAdvanced(NetworkPacket *pkt) = 0;

	virtual void ProcessData(NetworkPacket *pkt) = 0;
	virtual void QueueProcessData(NetworkPacket pkt) = 0;

	/*
		Sending
	*/
	virtual void Send(NetworkPacket *pkt) = 0;
	virtual void Send(NetworkPacket *pkt, bool relative) = 0;
	virtual void Send(session_t peer_id, NetworkPacket *pkt) = 0;
	virtual void Send(session_t peer_id, NetworkPacket *pkt, bool relative) = 0;
	virtual void HandleProxyCommand(NetworkPacket *pkt) = 0;
	virtual void OnlyDeleteSAO(session_t peer_id, uint16_t ID_) = 0;
	virtual void SendActiveObjectMessages(session_t peer_id, const std::string &datas, bool reliable = true) = 0;
	virtual void SendToMainServer(NetworkPacket *pkt) = 0;
	virtual void DoSendToEveryone(NetworkPacket *pkt, NetworkPacket *legacy_pkt, u16 SID) = 0;
	virtual void QueueSendTo(session_t peer, NetworkPacket pkt, bool reliable) = 0;

	virtual void SendPlayerHPOrDie(PlayerSAO *player, const PlayerHPChangeReason &reason) = 0;
	virtual void SendPlayerBreath(PlayerSAO *sao) = 0;
	virtual void SendInventory(PlayerSAO *playerSAO, bool incremental) = 0;
	virtual void SendMovePlayer(uint16_t peer_id, v3f overridepos = {72000, 0, 0}) = 0;
	virtual void SendPlayerSpeed(uint16_t peer_id, const v3f &added_vel) = 0;
	virtual void SendPlayerFov(uint16_t peer_id) = 0;
	virtual void SendMinimapModes(uint16_t peer_id, std::vector<MinimapMode> &modes, size_t wanted_mode) = 0;
	virtual void sendDetachedInventories(uint16_t peer_id, bool incremental) = 0;
	virtual void sendDetachedInventory(Inventory *inventory, const std::string &name, session_t peer_id) = 0;
	virtual bool SendBlock(uint16_t peer_id, const v3s16 &blockpos) = 0;
	virtual void SendBlockNoLock(uint16_t peer_id, MapBlock *block, u8 ver, u16 net_proto_version) = 0;
	virtual void SendBlocks(float dtime) = 0;
	virtual void SendEyeOffset(uint16_t peer_id, v3f first, v3f third) = 0;
	virtual void SendPlayerPrivileges(uint16_t peer_id) = 0;
	virtual void SendPlayerInventoryFormspec(uint16_t peer_id) = 0;
	virtual void SendPlayerFormspecPrepend(uint16_t peer_id) = 0;
	virtual void SendShowFormspecMessage(uint16_t peer_id, const std::string &formspec, const std::string &formname) = 0;
	virtual void SendHUDAdd(uint16_t peer_id, u32 id, HudElement *form) = 0;
	virtual void SendHUDRemove(uint16_t peer_id, u32 id) = 0;
	virtual void SendHUDChange(uint16_t peer_id, u32 id, HudElementStat stat, void *value) = 0;
	virtual void SendHUDSetFlags(uint16_t peer_id, u32 flags, u32 mask) = 0;
	virtual void SendHUDSetParam(uint16_t peer_id, u16 param, const std::string &value) = 0;
	virtual void SendSetSky(uint16_t peer_id, const SkyboxParams &params) = 0;
	virtual void SendSetSun(uint16_t peer_id, const SunParams &params) = 0;
	virtual void SendSetMoon(uint16_t peer_id, const MoonParams &params) = 0;
	virtual void SendSetStars(uint16_t peer_id, const StarParams &params) = 0;
	virtual void SendCloudParams(uint16_t peer_id, const CloudParams &params) = 0;
	virtual void SendOverrideDayNightRatio(uint16_t peer_id, bool do_override, float ratio) = 0;
	virtual void SendAddParticleSpawner(uint16_t peer_id, u16 protocol_version,
		const ParticleSpawnerParameters &p, u16 attached_id, u32 id) = 0;
	virtual void SendDeleteParticleSpawner(uint16_t peer_id, u32 id) = 0;
	virtual void SendSpawnParticle(uint16_t peer_id, u16 protocol_version,
		const ParticleParameters &p) = 0;
	virtual void SendActiveObjectRemoveAdd(RemoteClient *client, PlayerSAO *playersao) = 0;
	virtual void SendCSMRestrictionFlags(session_t peer_id) = 0;
	virtual void SendActiveObjectRemoveAdd_SLAVE(ClientDataHelper *client, PlayerSAO *playersao) = 0;
	virtual void SendRemoveObjectsToClient(u16 pid, bool keep_attached_ones) = 0;
	virtual void sendUpdatePlayerSaoList(u16 playerid, std::unordered_map<u16, u16> playerlistNsao) = 0;
	virtual void SendDisconnectToPlayer(u16 ID) = 0;

	/*
		Media
	*/
	virtual void fillMediaCache() = 0;
	virtual void sendMediaAnnouncement(uint16_t peer_id, const std::string &lang_code) = 0;
	virtual void sendRequestedMedia(uint16_t peer_id, const std::vector<std::string> &tosend) = 0;
	virtual bool dynamicAddMedia(const std::string &filepath, std::vector<RemotePlayer*> &sent_to) = 0;
	virtual bool addMediaFile(const std::string &filename, const std::string &filepath,
			std::string *filedata = nullptr, std::string *digest = nullptr) = 0;

	/*
		Time / shutdown / environment
	*/
	virtual void process_PlayerPos(RemotePlayer *player, PlayerSAO *playersao, NetworkPacket *pkt) = 0;
	virtual void setTimeOfDay(u32 time) = 0;
	virtual void onMapEditEvent(const MapEditEvent &event) = 0;
	virtual std::string getStatusString() = 0;
	virtual double getUptime() const = 0;
	virtual bool isShutdownRequested() const = 0;
	virtual void requestShutdown(const std::string &msg, bool reconnect, float delay = 0.0f) = 0;

	/*
		Sound
	*/
	virtual int32_t playSound(const SimpleSoundSpec &spec, const ServerSoundParams &params,
			bool ephemeral = false) = 0;
	virtual void stopSound(int32_t handle) = 0;
	virtual void fadeSound(int32_t handle, float step, float gain) = 0;
	virtual int32_t nextSoundId() = 0;

	/*
		Privileges / bans
	*/
	virtual std::set<std::string> getPlayerEffectivePrivs(const std::string &name) = 0;
	virtual bool checkPriv(const std::string &name, const std::string &priv) = 0;
	virtual void reportPrivsModified(const std::string &name = "") = 0;
	virtual void reportInventoryFormspecModified(const std::string &name) = 0;
	virtual void reportFormspecPrependModified(const std::string &name) = 0;
	virtual void setIpBanned(const std::string &ip, const std::string &name) = 0;
	virtual void unsetIpBanned(const std::string &ip_or_name) = 0;
	virtual std::string getBanDescription(const std::string &ip_or_name) = 0;

	/*
		Chat / notifications
	*/
	virtual void notifyPlayer(const char *name, const std::wstring &msg) = 0;
	virtual void notifyPlayers(const std::wstring &msg) = 0;
	virtual void handleChatInterfaceEvent(ChatEvent *evt) = 0;
	virtual std::wstring handleChat(const std::string &name, std::wstring wmessage_input,
		bool check_shout_priv = false, RemotePlayer *player = nullptr) = 0;
	virtual void handleAdminChat(const ChatEventChat *evt) = 0;
	virtual void printToConsoleOnly(const std::string &text) = 0;

	/*
		Particles
	*/
	virtual void spawnParticle(const std::string &playername, const ParticleParameters &p) = 0;
	virtual uint32_t addParticleSpawner(const ParticleSpawnerParameters &p,
		ServerActiveObject *attached, const std::string &playername) = 0;
	virtual void deleteParticleSpawner(const std::string &playername, u32 id) = 0;

	/*
		Inventory / scripting / defs (IGameDef overrides)
	*/
	virtual ServerInventoryManager *getInventoryMgr() const = 0;
	virtual ServerScripting *getScriptIface() = 0;
	virtual IItemDefManager* getItemDefManager() = 0;
	virtual const NodeDefManager* getNodeDefManager() = 0;
	virtual ICraftDefManager* getCraftDefManager() = 0;
	virtual u16 allocateUnknownNodeId(const std::string &name) = 0;
	virtual EmergeManager *getEmergeManager(uint16_t mapid = 0) = 0;
	virtual IWritableItemDefManager* getWritableItemDefManager() = 0;
	virtual NodeDefManager* getWritableNodeDefManager() = 0;
	virtual IWritableCraftDefManager* getWritableCraftDefManager() = 0;
	virtual std::string getWorldPath() const = 0;

	// Static: not part of the vtable, kept as-is.
	static std::string getBuiltinLuaPath();

	virtual bool isSingleplayer() = 0;
	virtual void setAsyncFatalError(const std::string &error) = 0;

	/*
		Formspec / HUD / player visuals
	*/
	virtual bool showFormspec(const char *name, const std::string &formspec, const std::string &formname) = 0;
	virtual Map & getMap() = 0;
	virtual ServerEnvironment & getEnv() = 0;
	virtual v3f findSpawnPos(uint16_t map_id = 0) = 0;
	virtual uint32_t hudAdd(RemotePlayer *player, HudElement *element) = 0;
	virtual bool hudRemove(RemotePlayer *player, uint32_t id) = 0;
	virtual bool hudChange(RemotePlayer *player, u32 id, HudElementStat stat, void *value) = 0;
	virtual bool hudSetFlags(RemotePlayer *player, u32 flags, u32 mask) = 0;
	virtual bool hudSetHotbarItemcount(RemotePlayer *player, s32 hotbar_itemcount) = 0;
	virtual void hudSetHotbarImage(RemotePlayer *player, const std::string &name) = 0;
	virtual void hudSetHotbarSelectedImage(RemotePlayer *player, const std::string &name) = 0;
	virtual Address getPeerAddress(session_t peer_id) = 0;
	virtual void setLocalPlayerAnimations(RemotePlayer *player, v2s32 animation_frames[4],
			f32 frame_speed) = 0;
	virtual void setPlayerEyeOffset(RemotePlayer *player, const v3f &first, const v3f &third) = 0;
	virtual void setSky(RemotePlayer *player, const SkyboxParams &params) = 0;
	virtual void setSun(RemotePlayer *player, const SunParams &params) = 0;
	virtual void setMoon(RemotePlayer *player, const MoonParams &params) = 0;
	virtual void setStars(RemotePlayer *player, const StarParams &params) = 0;
	virtual void setClouds(RemotePlayer *player, const CloudParams &params) = 0;
	virtual void overrideDayNightRatio(RemotePlayer *player, bool do_override, float brightness) = 0;

	/*
		con::PeerHandler implementation
	*/
	virtual void peerAdded(con::Peer *peer) = 0;
	virtual void deletingPeer(con::Peer *peer, bool timeout) = 0;

	/*
		Connection / auth
	*/
	virtual void DenySudoAccess(uint16_t peer_id) = 0;
	virtual void DenyAccess(uint16_t peer_id, AccessDeniedCode reason,
		const std::string &custom_reason = "", bool reconnect = false) = 0;
	virtual void acceptAuth(uint16_t peer_id, bool forSudoMode) = 0;
	virtual void DisconnectPeer(uint16_t peer_id) = 0;
	virtual bool getClientConInfo(uint16_t peer_id, con::rtt_stat_type type, float *retval) = 0;
	virtual bool getClientInfo(uint16_t peer_id, ClientInfo &ret) = 0;
	virtual void DisconnectAllPlayers(bool crashed = false) = 0;
	virtual RemoteClient* getClientNoEx(uint16_t peer_id, ClientState state_min = CS_Active) = 0;
	virtual RemoteClient* getClient(uint16_t peer_id, ClientState state_min = CS_Active) = 0;
	virtual std::string getPlayerName(uint16_t peer_id) = 0;
	virtual uint8_t getSerializationVersion(session_t peer_id) = 0;
	virtual u16 getProtocolOfThisPeer(session_t PeerID) = 0;
	virtual float getRecommendedSendInterval() = 0;
	virtual ClientDataHelper *getClientCDH(uint16_t p_id) = 0;
	virtual Translations *getTranslationLanguage(const std::string &lang_code) = 0;

	/*
		Player lifecycle
	*/
	virtual PlayerSAO *emergePlayer(const char *name, uint16_t peer_id, u16 proto_version) = 0;
	virtual void handlePeerChanges() = 0;
	virtual void InitializePlayer(session_t pid) = 0;
	virtual void DiePlayer(uint16_t peer_id, PlayerHPChangeReason &reason) = 0;
	virtual void RespawnPlayer(uint16_t peer_id) = 0;
	virtual void UpdateCrafting(RemotePlayer *player) = 0;
	virtual bool checkInteractDistance(RemotePlayer *player, const f32 d, const std::string &what) = 0;
	virtual PlayerSAO *getPlayerSAO(uint16_t peer_id) = 0;
	virtual PlayerSAO *InitClientByMainServer(u16 ID, ClientDataHelper *client) = 0;
	virtual void DeletePlayer(u16 p) = 0;
	virtual void DeleteClient(session_t peer_id, ClientDeletionReason reason) = 0;
	virtual u16 GetRandomIDforPlayer() = 0;
	virtual void QueuePlayerToInitialize(session_t p) = 0;
	virtual bool ExistsID(u16 ID) = 0;
	virtual void QueueOnJoinPlayer(PlayerSAO *sao, const char *name) = 0;
	virtual void HandleIDforServer(NetworkPacket *pkt) = 0;
	virtual void MakeThisASlaveServer(u16 id) = 0;
	virtual bool equalsPlayerIDwithPeerID(u16 PlayerID, session_t PeerID) = 0;

	/*
		Multi-map support
	*/
	virtual bool setPlayerOnMap(RemotePlayer *player, uint16_t mapid, v3f def_pos = {0, 0, 0}) = 0;
	virtual bool unSetPlayerOnMap(RemotePlayer *player) = 0;
	virtual std::list<RemotePlayer*> getPlayersInMap(uint16_t mapid) = 0;
	virtual void loadMapFiles() = 0;
	virtual void saveMapFiles() = 0;
	virtual bool createNewMap(const std::string mapname, uint16_t *mapid) = 0;
	virtual bool deleteMap(uint16_t mapid) = 0;
	virtual bool existsMap(uint16_t mapid) = 0;

	/*
		Master/slave server split
	*/
	virtual void AsyncRunStepSlave(bool initial_step) = 0;
	virtual void AsyncRunStepMain(bool initial_step) = 0;

	/*
		Node/block edits
	*/
	virtual void sendRemoveNode(v3s16 p, std::unordered_set<u16> *far_players = nullptr,
		float far_d_nodes = 100) = 0;
	virtual void sendAddNode(v3s16 p, MapNode n, std::unordered_set<u16> *far_players = nullptr,
		float far_d_nodes = 100, bool remove_metadata = true) = 0;
	virtual void sendMetadataChanged(const std::list<v3s16> &meta_updates, float far_d_nodes = 100) = 0;
	virtual void SetBlocksNotSent(std::map<v3s16, MapBlock *>& block) = 0;
	virtual void sendDefinitions() = 0;

	/*
		Misc getters
	*/
	virtual bool isCompatPlayerModel(const std::string &model_name) = 0;
	virtual const std::vector<std::string> getCompatPlayerModels() = 0;
	virtual SubgameSpec getGameSpec() = 0;
};

/*
	Factory functions the .so must export with extern "C" linkage.
	This is the actual ABI boundary: name-mangling-free entry points that
	hand back / take back an IServer* whose virtual calls resolve through
	the vtable, which IS stable as long as both sides:
	  - agree on IServer's method order (i.e. build against this same header)
	  - are built with compatible compilers/ABI (e.g. both GCC, same major
	    version family, or both Clang, etc.)

	extern "C" {
		typedef IServer* (*CreateServerFunc)(const std::string &path,
			const SubgameSpec &gamespec, bool simple_singleplayer_mode,
			Address addr, bool dedicated);
		typedef void (*DestroyServerFunc)(IServer *server);
	}

	// In the .so:
	extern "C" IServer* create_server(const std::string &path,
			const SubgameSpec &gamespec, bool ssm, Address addr, bool dedicated) {
		return new Server(path, gamespec, ssm, addr, dedicated);
	}
	extern "C" void destroy_server(IServer *server) {
		delete server;
	}
*/
