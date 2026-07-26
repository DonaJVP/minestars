#include "handles.h"
#include "../inventory.h"
#include "../server/player_sao.h"
#include "../util/pointedthing.h"
#include "../remoteplayer.h"
#include "../serverenvironment.h"
#include "../nodedef.h"
#include "../addons/cblks_def.hpp"

void _on_place_(ItemStack &item, PlayerSAO *sao, PointedThing pointed) {
    if (!sao) {
        return;
    }
    ServerEnvironment *env = sao->getEnv();
    RemotePlayer *player = sao->getPlayer();
    if (pointed.type == POINTEDTHING_NODE) {
        if (player->sneakpressed) {
            MapNode n;
            content_t i;
            env->getGameDef()->getNodeDefManager()->getId(item.name, i);
            n.setContent(i);
            env->setNode(pointed.node_abovesurface, n, sao->getMapId());
        } else {
            bool pSuccess;
            MapNode n = env->getMap(sao->getMapId()).getNode(pointed.node_undersurface, &pSuccess);
            _callback_ACT1 _oR = env->getGameDef()->getNodeDefManager()->get(n.getContent()).on_rightclick;
            if (_oR != nullptr) {
                _oR(item, sao, pointed);
            } else {
                MapNode n;
                content_t i;
                env->getGameDef()->getNodeDefManager()->getId(item.name, i);
                n.setContent(i);
                env->setNode(pointed.node_abovesurface, n, sao->getMapId());
            }
        }
    } else if (pointed.type == POINTEDTHING_OBJECT) {
        ServerActiveObject *pointed_object = sao->getEnv()->getActiveObject(pointed.object_id);
        if (pointed_object->isGone())
            return;
        actionstream << sao->getPlayerID() << " right-clicks object " << pointed.object_id << ": " << pointed_object->getDescription() << std::endl;
        ///
        (reinterpret_cast<bool(*)(ItemStack, PlayerSAO*, PointedThing)>(AddonsCallbacks[CALLBACK_ONSECONDARYUSE]))(item, sao, pointed);
        pointed_object->rightClick(sao);
        //sao->getEnv()->getGameDef()->SendInventory(sao, true);
    }
}

bool _on_dig_(ItemStack &item, PlayerSAO *sao, PointedThing pointed) {
    ServerEnvironment *env = sao->getEnv();
    env->removeNode(pointed.node_undersurface, sao->getMapId());
    return true;
}

_on_place on_place_root = &_on_place_;
_on_dig on_dig_root = &_on_dig_;
