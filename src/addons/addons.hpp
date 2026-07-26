// Addons engine, a replacement for lua/clua.
#include <cstdint>
#include <vector>
#include <string>

#pragma once

class servAddons;
class Server;

#ifdef __cplusplus
extern "C" {
#endif
void registerCallback(uint64_t ID, uint64_t funcPtr);
void registerMediaPath(std::string path);
#ifdef __cplusplus
}
#endif

class hyperMod {
public:
    virtual ~hyperMod() = default;
    virtual void init(const std::string &path, Server *server, servAddons *service) = 0;
    virtual void onStop(Server *server) = 0;
};

struct AddonInfo {
    const char* author;
    const char* name;
    const char* version;
};

struct _ADDONinstance {
    hyperMod *link = nullptr;
    uint64_t initialized = false;
    std::string name;
};

class servAddons {
public:
    servAddons(Server *s): serv(s) {}
    bool initializeSet(const std::string &pathToDir);
    void loadAddon(const std::string &path);
    bool unloadAddon(const std::string &name);
    std::vector<std::string> getAddonsList();
    // ASMJIT REFERENCES
    void preCompileCallbacks(); // Prepare callbacks slots
    void setReady(); // Compile callbacks
    // hyperMod Interference
    void saveCallback(uint64_t ID, uint64_t ptr);
    std::vector<std::vector<uint64_t>> CBregistered;
    void setCallbackStatus(bool status);
    void insertMediaPath(const std::string path);
    inline std::vector<std::string> getMediaPaths() { return mediaPaths; }
private:
    Server *serv;
    std::vector<std::string> mediaPaths;
    std::vector<_ADDONinstance> addons;
};

extern servAddons *SERVER_ADDONS;

