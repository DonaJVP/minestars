#include <string>
#include <set>
#include <cstdint>

#pragma once

struct ItemStack;
class PlayerSAO;
#include "../util/pointedthing.h"

using _formatChatMessage = std::string(*)(const std::string&, const std::string&);
using _getAuth = bool(*)(const std::string&, std::string*, std::set<std::string>*, int64_t*);
using _on_place = void(*)(ItemStack&, PlayerSAO*, PointedThing);
using _on_dig = bool(*)(ItemStack&, PlayerSAO*, PointedThing);

extern _formatChatMessage formatChatMessage;
extern _getAuth getAuth;
extern _on_place on_place_root;
extern _on_dig on_dig_root;
