#include "handles.h"
#include "../database/database.h"
#include "../auth.h"
#include "../log.h"

AuthDatabase *data = nullptr;

void _AUTH__setDatabaseObject(AuthDatabase *obj) {
    data = obj;
};

bool auth_handler(const std::string &playername, std::string *dst_password, std::set<std::string> *dst_privs, int64_t *dst_last_login) {
    bool _ret = false;
    // Just read.
    AuthEntry entry;
    bool success = data->getAuth(playername, entry);
    if (!success) {
        warningstream << "Tried to read auth data from player " << playername << ", which doesn't exist" << std::endl;
        return false;
    }
    if (dst_password) {
        *dst_password = entry.password;
    }
    if (dst_privs) {
        std::set<std::string> privs;
        for (std::string &priv: entry.privileges) {
            privs.insert(priv);
        }
        *dst_privs = privs;
    }
    if (dst_last_login) {
        *dst_last_login = entry.last_login;
    }
    return true;
}

_getAuth getAuth = &auth_handler;
