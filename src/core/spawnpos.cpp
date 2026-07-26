/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, Donatto J. Viveros. P. <donatto555@gmail.com>
 * Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../server.h"
#include "../map.h"
#include "../constants.h"
#include "../mapblock.h"
#include "../server/serveractiveobject.h"
#include "../settings.h"
#include "../nodedef.h"
#include "../emerge.h"
#include "../mapgen/mapgen.h"
#include "../mapgen/mg_biome.h"
#include "../defaultsettings.h"
#include "../irr_v3d.h"

v3f Server::findSpawnPos(uint16_t map_id)
{
    ServerMap &map = m_env->getServerMap(map_id);
    v3f nodeposf;
    if (g_settings->getV3FNoEx("static_spawnpoint", nodeposf) ||
        m_env->getWorldSpawnpoint(nodeposf))
        return nodeposf * BS;

    bool is_good = false;
    // Limit spawn range to mapgen edges (determined by 'mapgen_limit')
    s32 range_max = map.getMapgenParams()->getSpawnRangeMax();

    // Try to find a good place a few times
    for (s32 i = 0; i < 4000 && !is_good; i++) {
        s32 range = MYMIN(1 + i, range_max);
        // We're going to try to throw the player to this position
        v2s16 nodepos2d = v2s16(
            -range + (myrand() % (range * 2)),
                                -range + (myrand() % (range * 2)));
        // Get spawn level at point
        s16 spawn_level = Maps.at(map_id)->m_emerge->getSpawnLevelAtPoint(nodepos2d);
        // Continue if MAX_MAP_GENERATION_LIMIT was returned by the mapgen to
        // signify an unsuitable spawn position, or if outside limits.
        if (spawn_level >= MAX_MAP_GENERATION_LIMIT ||
            spawn_level <= -MAX_MAP_GENERATION_LIMIT)
            continue;

        v3s16 nodepos(nodepos2d.X, spawn_level, nodepos2d.Y);
        // Consecutive empty nodes
        s32 air_count = 0;

        // Search upwards from 'spawn level' for 2 consecutive empty nodes, to
        // avoid obstructions in already-generated mapblocks.
        // In ungenerated mapblocks consisting of 'ignore' nodes, there will be
        // no obstructions, but mapgen decorations are generated after spawn so
        // the player may end up inside one.
        for (s32 i = 0; i < 8; i++) {
            v3s16 blockpos = getNodeBlockPos(nodepos);
            map.emergeBlock(blockpos, true);
            content_t c = map.getNode(nodepos).getContent();

            // In generated mapblocks allow spawn in all 'airlike' drawtype nodes.
            // In ungenerated mapblocks allow spawn in 'ignore' nodes.
            if (m_nodedef->get(c).drawtype == NDT_AIRLIKE || c == CONTENT_IGNORE) {
                air_count++;
                if (air_count >= 2) {
                    // Spawn in lower empty node
                    nodepos.Y--;
                    nodeposf = intToFloat(nodepos, BS);
                    // Don't spawn the player outside map boundaries
                    if (objectpos_over_limit(nodeposf))
                        // Exit this loop, positions above are probably over limit
                        break;

                    // Good position found, cause an exit from main loop
                    is_good = true;
                    break;
                }
            } else {
                air_count = 0;
            }
            nodepos.Y++;
        }
    }

    if (is_good)
        return nodeposf;

    // No suitable spawn point found, return fallback 0,0,0
    return v3f(0.0f, 0.0f, 0.0f);
}

