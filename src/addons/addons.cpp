#include "addons.hpp"
#include <cstdint>
#include <dlfcn.h>
#include <string>
#include "../filesys.h"
#include "../server.h"

bool AddonsCallbackStatus = true;

using GetModInfoFn = const AddonInfo*(*)();
using __CreateModFn = hyperMod*(*)(Server*);
servAddons *SERVER_ADDONS = nullptr;

//servAddons
void servAddons::loadAddon(const std::string& path) {
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        throw std::runtime_error(dlerror());
    }
    dlerror();
    auto a = (GetModInfoFn)dlsym(handle, "ADDON_INFO");
    if (!a) {
        dlclose(handle);
        errorstream << FUNCTION_NAME << ": Not addon information about " << path << std::endl;
        abort();
    }
    dlerror();
    const AddonInfo *info = a();
    // Copycat info.
    AddonInfo i = {
        std::move(info->author),
        std::move(info->name),
        std::move(info->version)
    };
    
    infostream << FUNCTION_NAME << ": Loading addon " << i.name << std::endl;
    
    //Get pointer
    __CreateModFn init = (__CreateModFn)dlsym(handle, "init0");
    hyperMod *rMod = init(serv);
    dlerror();
    _ADDONinstance Mod;
    Mod.initialized = true;
    Mod.link = rMod;
    Mod.name = std::string(i.name, strlen(i.name));
    addons.push_back(Mod);
    
    rMod->init(path, serv, this);
}

bool servAddons::unloadAddon(const std::string &name) {
    _ADDONinstance *addon = nullptr;
    uint32_t counter = 0;
    while (counter < addons.size()) {
        addon = &addons.at(counter);
        if (addon->name == name) {
            // Disable it
            addon->link->onStop(serv);
            addon->initialized = false;
            goto Found;
        }
        counter++;
    }
    notFound:
        return false;
    Found:
        return true;
}

#include <iostream>
static void ___TEST(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e) {
    std::cout << "YESHHH" << std::endl;
}

bool servAddons::initializeSet(const std::string &pathToDir) {
    SERVER_ADDONS = this;
    // Fill callbacks list.
    for (int i = 0; i < 20; i++)
        CBregistered.push_back(std::vector<uint64_t>( { } ));
    // Secure.
    fs::CreateAllDirs(pathToDir);
    // List every folder in this path.
    for (fs::DirListNode &i: fs::GetDirListing(pathToDir)) {
        if (i.dir) {
            // Must check if the .so exists
            std::string data;
            bool loadedsuccessfully = fs::ReadFile(pathToDir+"/"+i.name+"/addon.so", data);
            if (!loadedsuccessfully) {
                warningstream << FUNCTION_NAME << ": Addon might have been set badly: " << pathToDir << "/" << i.name << std::endl;
                return false;
            } else {
                loadAddon(pathToDir+"/"+i.name+"/addon.so");
            }
        } else {
            loadAddon(pathToDir+"/addon.so");
        }
    }
    return true;
}

std::vector<std::string> servAddons::getAddonsList() {
    std::vector<std::string> aList;
    for (_ADDONinstance &addon: addons) {
        if (addon.initialized)
            aList.push_back(addon.name);
    }
    return aList;
} 

void servAddons::setCallbackStatus(bool status) {
    AddonsCallbackStatus = status;
}

void servAddons::insertMediaPath(const std::string path) {
    mediaPaths.push_back(path);
}

// hyperMod //

extern "C" void registerCallback(uint64_t ID, uint64_t funcPtr) {
    std::vector<uint64_t> *slot = &SERVER_ADDONS->CBregistered.at(ID);
    slot->push_back(funcPtr);
}

extern "C" void registerMediaPath(std::string path) {
    SERVER_ADDONS->insertMediaPath(path);
}
