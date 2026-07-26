--[[
    X Obsidianmese. Adds obsidian and mese tools and items.
    Copyright (C) 2024 SaKeL

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
--]]

x_obsidianmese = {
    mod = {
        ethereal = minetest.get_modpath('ethereal')
    },
    settings = {
        x_obsidianmese_chest = minetest.settings:get_bool('x_obsidianmese_chest', true),
        x_obsidianmese_sword_engraved_recipe =
            minetest.settings:get_bool('x_obsidianmese_sword_engraved_recipe', true)
    },
    capitator_tree_names = {
        'default:cactus',
        'x_farming:cactus',
    },
    path_node_defs = {}
}

minetest.register_on_mods_loaded(function()
    for key, value in pairs(minetest.registered_nodes) do
        if minetest.get_item_group(key, 'tree') > 0 then
            table.insert(x_obsidianmese.capitator_tree_names, key)
        end
    end

    x_obsidianmese.register_capitator()
end)

-- save how many bullets owner fired
x_obsidianmese.fired_table = {}
local enable_particles = minetest.settings:get_bool('enable_particles', true)

local function bound(x, minb, maxb)
    if x < minb then
        return minb
    elseif x > maxb then
        return maxb
    else
        return x
    end
end

--- Punch damage calculator.
-- By default, this just calculates damage in the vanilla way. Switch it out for something else to change the default
-- damage mechanism for mobs.
-- @param ObjectRef player
-- @param ?ObjectRef puncher
-- @param number time_from_last_punch
-- @param table tool_capabilities
-- @param ?vector direction
-- @param ?Id attacker
-- @return number The calculated damage
-- @author raymoo
function x_obsidianmese.damage_calculator(player, puncher, tflp, caps, direction, attacker)
    local a_groups = player:get_armor_groups() or {}
    local full_punch_interval = caps.full_punch_interval or 1.4
    local time_prorate = bound(tflp / full_punch_interval, 0, 1)

    local damage = 0
    for group, damage_rating in pairs(caps.damage_groups or {}) do
        local armor_rating = a_groups[group] or 0
        damage = damage + damage_rating * (armor_rating / 100)
    end

    return math.floor(damage * time_prorate)
end

-- particles
function x_obsidianmese.add_effects(pos)

    if not enable_particles then return end

    if minetest.has_feature({ dynamic_add_media_table = true, particlespawner_tweenable = true }) then
        -- new syntax, after v5.6.0
        local particlespawner_def = {
            amount = 20,
            time = 5,
            size = {
                min = 0.5,
                max = 1.5,
            },
            exptime = 5,
            pos = {
                min = vector.new({ x = pos.x - 1.5, y = pos.y, z = pos.z - 1.5 }),
                max = vector.new({ x = pos.x + 1.5, y = pos.y + 1.5, z = pos.z + 1.5 }),
            },
            attract = {
                kind = 'point',
                strength = math.random(10, 30) / 100,
                origin = vector.new({ x = pos.x, y = pos.y, z = pos.z })
            },
            texture = {
                name = 'x_obsidianmese_chest_particle.png',
                alpha_tween = {
                    0.5, 1,
                    style = 'fwd',
                    reps = 1
                }
            },
            radius = { min = 1, max = 1.5, bias = 1 },
            glow = 6
        }

        minetest.add_particlespawner(particlespawner_def)
    else
        local nodes = minetest.find_nodes_in_area(
            vector.subtract(pos, 2),
            vector.add(pos, 2),
            { 'air' }
        )

        if #nodes == 0 then
            return
        end

        for i = 1, 10, 1 do
            local pos_random = nodes[math.random(1, #nodes)]
            local x = pos.x - pos_random.x
            local y = pos_random.y - pos.y
            local z = pos.z - pos_random.z
            local rand1 = (math.random(1, 10) / 10) * -1
            local rand2 = math.random(10, 500) / 100
            local rand3 = math.random(50, 150) / 100

            minetest.after(rand2, function()
                minetest.add_particle({
                    pos = pos_random,
                    velocity = vector.divide({ x = x, y = 1 - y, z = z }, 4),
                    acceleration = vector.divide({ x = 0, y = rand1, z = 0 }, 4),
                    expirationtime = 4.5,
                    size = rand3,
                    texture = 'x_obsidianmese_chest_particle.png',
                    glow = 6,
                    collisiondetection = true,
                    collision_removal = true
                })
            end)
        end
    end
end

-- check for player near by to activate particles
function x_obsidianmese.check_around_radius(pos)
    local player_near = false

    for _, obj in ipairs(minetest.get_objects_inside_radius(pos, 16)) do
        if obj:is_player() then
            player_near = true
            break
        end
    end

    return player_near
end

-- check if within physical map limits (-30911 to 30927)
function x_obsidianmese.within_limits(pos, radius)
    if (pos.x - radius) > -30913
    and (pos.x + radius) < 30928
    and (pos.y - radius) > -30913
    and (pos.y + radius) < 30928
    and (pos.z - radius) > -30913
    and (pos.z + radius) < 30928 then
        return true -- within limits
    end

    return false -- beyond limits
end

-- remember how many bullets player fired i.e. {SaKeL: 1,...}
function x_obsidianmese.sync_fired_table(owner)
    if x_obsidianmese.fired_table[owner] ~= nil then
        if x_obsidianmese.fired_table[owner] < 0 then
            x_obsidianmese.fired_table[owner] = 0
        else
            x_obsidianmese.fired_table[owner] = x_obsidianmese.fired_table[owner] - 1
        end
    end
end

function x_obsidianmese.fire_sword(itemstack, user, pointed_thing)
    if not user:get_player_control().RMB then return end

    local speed = 8
    local pos = user:get_pos()
    local v = user:get_look_dir()
    local player_name = user:get_player_name()

    if not x_obsidianmese.fired_table[player_name] or x_obsidianmese.fired_table[player_name] < 0 then
        x_obsidianmese.fired_table[player_name] = 0
    end

    if x_obsidianmese.fired_table[player_name] >= 1 then
        minetest.chat_send_player(player_name, 'You can shoot 1 shot at the time!')
        return itemstack
    end

    x_obsidianmese.fired_table[player_name] = x_obsidianmese.fired_table[player_name] + 1

    -- adjust position from where the bullet will be fired based on the look direction
    -- prevents hitting the node when looking/shooting down from the edge
    pos.x = pos.x + v.x
    pos.z = pos.z + v.z
    if v.y > 0.4 or v.y < -0.4 then
        pos.y = pos.y + v.y
    else
        pos.y = pos.y + 1
    end

    -- play shoot attack sound
    minetest.sound_play('x_obsidianmese_throwing', {
        pos = pos,
        gain = 1.0, -- default
        max_hear_distance = 10,
    })

    local obj = minetest.add_entity(pos, 'x_obsidianmese:sword_bullet')

    if not obj then
        return
    end

    local ent = obj:get_luaentity()

    if ent then
        ent._owner = player_name

        v.x = v.x * speed
        v.y = v.y * speed
        v.z = v.z * speed

        obj:set_velocity(v)
    end

    -- wear tool
    local wdef = itemstack:get_definition()
    itemstack:add_wear(65535 / (150 - 1))

    -- Tool break sound
    if itemstack:get_count() == 0 and wdef.sound and wdef.sound.breaks then
        minetest.sound_play(wdef.sound.breaks, { pos = pointed_thing.above, gain = 0.5 })
    end

    return itemstack
end

function x_obsidianmese.add_wear(itemstack, pos)
    -- wear tool
    local wdef = itemstack:get_definition()
    itemstack:add_wear(65535 / (400 - 1))
    -- Tool break sound
    if itemstack:get_count() == 0 and wdef.sound and wdef.sound.breaks then
        minetest.sound_play(wdef.sound.breaks, { pos = pos, gain = 0.5 })
    end

    return itemstack
end

function x_obsidianmese.pick_engraved_place(itemstack, placer, pointed_thing)
    local idx = placer:get_wield_index() + 1 -- item to right of wielded tool
    local inv = placer:get_inventory() --[[@as InvRef]]
    -- stack to right of tool
    local stack = inv:get_stack('main', idx)
    local stack_name = stack:get_name()
    local under = pointed_thing.under
    local above = pointed_thing.above
    local temp_stack

	if pointed_thing.type == "node" then
		local pos = pointed_thing.under
		if stack_name == "" then
			return itemstack
		end
		local pointed_node = core.get_node(pos)
		if not pointed_node then return itemstack end
		local def = core.registered_nodes[pointed_node.name]
		if def.on_rightclick then
			return def.on_rightclick(pos, pointed_node, placer, itemstack, pointed_thing)
		end
		local node = core.registered_nodes[stack_name]
		if node then
			local itm = core.item_place(stack, placer, pointed_thing)
			inv:set_stack("main", idx, itm or ItemStack(""))
		end
	end


--[[
    -- handle nodes
    if pointed_thing.type == 'node' then
        local pos = minetest.get_pointed_thing_position(pointed_thing)

        if not pos or stack_name == '' then
            return itemstack
        end

        local pointed_node = minetest.get_node(pos)
        local pointed_node_def = minetest.registered_nodes[pointed_node.name]

        if not pointed_node then
            return itemstack
        end

        -- check if we have to use default on_place first
        if pointed_node_def.on_rightclick then
            return pointed_node_def.on_rightclick(pos, pointed_node, placer, itemstack, pointed_thing)
        end

        local udef = minetest.registered_nodes[stack_name] or minetest.registered_items[stack_name]

        if udef and udef.on_place then
            temp_stack = udef.on_place(stack, placer, pointed_thing) or stack
            inv:set_stack('main', idx, temp_stack)

            -- play sound
            if udef.sounds then
                if udef.sounds.place then
                    minetest.sound_play(udef.sounds.place.name, {
                        gain = udef.sounds.place.gain or 1
                    })
                end
            end

            return itemstack
        elseif udef and udef.on_use then
            temp_stack = udef.on_use(stack, placer, pointed_thing) or stack
            inv:set_stack('main', idx, temp_stack)

            return itemstack
        end

        -- handle default torch placement
        if stack_name == 'default:torch' then
            local wdir = minetest.dir_to_wallmounted(vector.subtract(under, above))
            local fakestack = stack

            if wdir == 0 then
                fakestack:set_name('default:torch_ceiling')
            elseif wdir == 1 then
                fakestack:set_name('default:torch')
            else
                fakestack:set_name('default:torch_wall')
            end

            temp_stack = minetest.item_place(fakestack, placer, pointed_thing, wdir)

            temp_stack:set_name('default:torch')
            inv:set_stack('main', idx, temp_stack)

            -- play sound
            if udef.sounds then
                if udef.sounds.place then
                    minetest.sound_play(udef.sounds.place.name, {
                        gain = udef.sounds.place.gain or 1
                    })
                end
            end

            return itemstack
        end

        -- if everything else fails use default on_place
        stack = minetest.item_place(stack, placer, pointed_thing)
        inv:set_stack('main', idx, stack)

        -- play sound
        if udef.sounds then
            if udef.sounds.place then
                minetest.sound_play(udef.sounds.place.name, {
                    gain = udef.sounds.place.gain or 1
                })
            end
        end

        return itemstack
    end

    return itemstack--]]

	return itemstack
end

function x_obsidianmese.shovel_place(itemstack, placer, pointed_thing)
    local pt = pointed_thing

    -- check if pointing at a node
    if not pt then
        return itemstack
    end

    if pt.type ~= 'node' then
        return itemstack
    end

    local pos = minetest.get_pointed_thing_position(pointed_thing)

    if not pos then
        return itemstack
    end

    local pointed_node = minetest.get_node(pos)
    local pointed_node_def = minetest.registered_nodes[pointed_node.name]
    if pointed_node_def and pointed_node_def.on_rightclick then
        return pointed_node_def.on_rightclick(pos, pointed_node, placer, itemstack, pointed_thing)
    end

    local under = minetest.get_node(pt.under)
    local p = { x = pt.under.x, y = pt.under.y + 1, z = pt.under.z }
    local above = minetest.get_node(p)

    -- return if any of the nodes is not registered
    if not minetest.registered_nodes[under.name] then
        return itemstack
    end

    if not minetest.registered_nodes[above.name] then
        return itemstack
    end

    -- check if the node above the pointed thing is air
    if above.name ~= 'air' then
        return itemstack
    end

    if minetest.is_protected(pt.under, placer:get_player_name()) then
        minetest.record_protection_violation(pt.under, placer:get_player_name())
        return itemstack
    end

    if (under.name == 'default:dirt' or under.name == 'farming:soil' or under.name == 'farming:soil_wet')
        and under.name ~= 'x_obsidianmese:path_dirt'
    then
        -- dirt path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_dirt' })

    elseif (under.name == 'default:dirt_with_grass' or under.name == 'default:dirt_with_grass_footsteps')
        and under.name ~= 'x_obsidianmese:path_grass'
    then
        -- grass path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_grass' })

    elseif under.name == 'default:dirt_with_rainforest_litter'
        and under.name ~= 'x_obsidianmese:path_dirt_with_rainforest_litter'
    then
        -- rainforest litter path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_dirt_with_rainforest_litter' })

    elseif under.name == 'default:dirt_with_snow'
        and under.name ~= 'x_obsidianmese:path_dirt_with_snow'
    then
        -- dirt with snow path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_dirt_with_snow' })

    elseif under.name == 'default:dirt_with_dry_grass'
        and under.name ~= 'x_obsidianmese:path_dirt_with_dry_grass'
    then
        -- dirt with dry grass path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_dirt_with_dry_grass' })

    elseif under.name == 'default:dirt_with_coniferous_litter'
        and under.name ~= 'x_obsidianmese:path_dirt_with_coniferous_litter'
    then
        -- dirt with coniferous litter path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_dirt_with_coniferous_litter' })

    elseif under.name == 'default:dry_dirt'
        and under.name ~= 'x_obsidianmese:path_dry_dirt'
    then
        -- dry dirt path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_dry_dirt' })

    elseif under.name == 'default:dry_dirt_with_dry_grass'
        and under.name ~= 'x_obsidianmese:path_dry_dirt_with_dry_grass'
    then
        -- dry dirt with dry grass path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_dry_dirt_with_dry_grass' })

    elseif under.name == 'default:permafrost'
        and under.name ~= 'x_obsidianmese:path_permafrost'
    then
        -- permafrost path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_permafrost' })

    elseif under.name == 'default:permafrost_with_stones'
        and under.name ~= 'x_obsidianmese:path_permafrost_with_stones'
    then
        -- permafrost with stones path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_permafrost_with_stones' })

    elseif under.name == 'default:permafrost_with_moss'
        and under.name ~= 'x_obsidianmese:path_permafrost_with_moss'
    then
        -- permafrost with moss path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_permafrost_with_moss' })

    elseif under.name == 'default:sand'
        and under.name ~= 'x_obsidianmese:path_sand'
    then
        -- sand path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_sand' })

    elseif under.name == 'default:desert_sand'
        and under.name ~= 'x_obsidianmese:path_desert_sand'
    then
        -- desert sand path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_desert_sand' })

    elseif under.name == 'default:silver_sand'
        and under.name ~= 'x_obsidianmese:path_silver_sand'
    then
        -- silver sand path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_silver_sand' })

    elseif under.name == 'default:snowblock'
        and under.name ~= 'x_obsidianmese:path_snowblock'
    then
        -- snow path
        minetest.set_node(pt.under, { name = 'x_obsidianmese:path_snowblock' })

    elseif x_obsidianmese.mod.ethereal then
        x_obsidianmese.place_path_ethereal(under, pt.under)

    else
        -- New API approach
        local path_def = x_obsidianmese.path_node_defs[under.name]

        if not path_def then
            return
        end

        if path_def.path_node_name then
            minetest.set_node(pt.under, { name = path_def.path_node_name })
        end
    end

    -- play sound
    minetest.sound_play('default_dig_crumbly', {
        pos = pt.under,
        gain = 0.5
    })

    -- add wear
    if not minetest.settings:get_bool('creative_mode')
        or not minetest.check_player_privs(placer:get_player_name(), { creative = true })
    then
        itemstack = x_obsidianmese.add_wear(itemstack)
    end

    return itemstack
end

-- axe dig upwards
function x_obsidianmese.dig_up(pos, node, digger)
    if not digger then
        return
    end

    local wielditemname = digger:get_wielded_item():get_name()
    local whitelist = {
        ['x_obsidianmese:axe'] = true,
        ['x_obsidianmese:enchanted_axe_durable'] = true,
        ['x_obsidianmese:enchanted_axe_fast'] = true
    }

    if not whitelist[wielditemname] then
        return
    end

    local np = { x = pos.x, y = pos.y + 1, z = pos.z }
    local nn = minetest.get_node(np)

    if nn.name == node.name then
        local branches_pos = minetest.find_nodes_in_area(
            { x = np.x - 1, y = np.y, z = np.z - 1 },
            { x = np.x + 1, y = np.y + 1, z = np.z + 1 },
            node.name
        )

        minetest.node_dig(np, nn, digger)

        -- add particles only when not too far
        minetest.add_particlespawner({
            amount = math.random(1, 3),
            time = 0.5,
            minpos = { x = np.x - 0.7, y = np.y, z = np.z - 0.7 },
            maxpos = { x = np.x + 0.7, y = np.y + 0.75, z = np.z + 0.7 },
            minvel = { x = -0.5, y = -4, z = -0.5 },
            maxvel = { x = 0.5, y = -2, z = 0.5 },
            minacc = { x = -0.5, y = -4, z = -0.5 },
            maxacc = { x = 0.5, y = -2, z = 0.5 },
            minexptime = 0.5,
            maxexptime = 1,
            minsize = 0.5,
            maxsize = 2,
            collisiondetection = true,
            node = { name = nn.name }
        })

        if #branches_pos > 0 then
            for i = 1, #branches_pos do
                -- prevent infinite loop when node protected
                if minetest.is_protected(branches_pos[i], digger:get_player_name()) then
                    break
                end

                x_obsidianmese.dig_up(
                    { x = branches_pos[i].x, y = branches_pos[i].y - 1, z = branches_pos[i].z },
                    node,
                    digger
                )
            end
        end
    end
end

function x_obsidianmese.register_capitator()
    local trees = x_obsidianmese.capitator_tree_names

    for i = 1, #trees do
        local ndef = minetest.registered_nodes[trees[i]]

        if ndef then
            local prev_after_dig = ndef.after_dig_node
            local func = function(pos, node, metadata, digger)
                x_obsidianmese.dig_up(pos, node, digger)
            end

            if prev_after_dig then
                func = function(pos, node, metadata, digger)
                    prev_after_dig(pos, node, metadata, digger)
                    x_obsidianmese.dig_up(pos, node, digger)
                end
            end

            minetest.override_item(trees[i], { after_dig_node = func })
        end
    end
end

-- Taken from WorldEdit
-- Determines the axis in which a player is facing, returning an axis ('x', 'y', or 'z') and the sign (1 or -1)
function x_obsidianmese.player_axis(player)
    local dir = player:get_look_dir()
    local x, y, z = math.abs(dir.x), math.abs(dir.y), math.abs(dir.z)
    if x > y then
        if x > z then
            return 'x', dir.x > 0 and 1 or -1
        end
    elseif y > z then
        return 'y', dir.y > 0 and 1 or -1
    end
    return 'z', dir.z > 0 and 1 or -1
end

function x_obsidianmese.hoe_on_use(itemstack, user, pointed_thing)
    local pt = pointed_thing
    -- check if pointing at a node
    if not pt then
        return
    end

    if pt.type ~= 'node' then
        return
    end

    local under = minetest.get_node(pt.under)
    local above = minetest.get_node(pt.above)

    -- return if any of the nodes is not registered
    if not minetest.registered_nodes[under.name] then
        return
    end
    if not minetest.registered_nodes[above.name] then
        return
    end

    -- check if the node above the pointed thing is air
    if above.name ~= 'air' then
        return
    end

    -- check if pointing at soil
    if minetest.get_item_group(under.name, 'soil') ~= 1 then
        return
    end

    -- check if (wet) soil defined
    local regN = minetest.registered_nodes
    if regN[under.name].soil == nil or regN[under.name].soil.wet == nil or regN[under.name].soil.dry == nil then
        return
    end

    -- turn the node into soil and play sound
    minetest.set_node(pt.under, { name = regN[under.name].soil.dry })
    minetest.sound_play('default_dig_crumbly', {
        pos = pt.under,
        gain = 0.5,
    })

    minetest.add_particlespawner({
        amount = 10,
        time = 0.5,
        minpos = { x = pt.above.x - 0.4, y = pt.above.y - 0.4, z = pt.above.z - 0.4 },
        maxpos = { x = pt.above.x + 0.4, y = pt.above.y - 0.5, z = pt.above.z + 0.4 },
        minvel = { x = 0, y = 1, z = 0 },
        maxvel = { x = 0, y = 2, z = 0 },
        minacc = { x = 0, y = -4, z = 0 },
        maxacc = { x = 0, y = -8, z = 0 },
        minexptime = 1,
        maxexptime = 1.5,
        node = { name = regN[under.name].soil.dry },
        collisiondetection = true,
        object_collision = true,
    })
end

function x_obsidianmese.register_path_node(self, defs)
    if type(defs) ~= 'table' then
        minetest.log('warning', '[x_obsidianmese] Not registering path nodes due to incorrect node definition!')
        return
    end

    for key, value in pairs(defs) do
        local def = table.copy(value)
        local name = def.mod_origin .. ':path_' .. def.name

        def.path_node_name = name

        if not self.path_node_defs[key] then
            self.path_node_defs[key] = def
        end

        minetest.register_node(name, {
            description = def.description,
            short_description = def.short_description or def.description,
            drawtype = 'nodebox',
            tiles = def.tiles,
            use_texture_alpha = 'clip',
            is_ground_content = false,
            paramtype = 'light',
            node_box = {
                type = 'fixed',
                fixed = { -1 / 2, -1 / 2, -1 / 2, 1 / 2, 1 / 2 - 1 / 16, 1 / 2 },
            },
            collision_box = {
                type = 'fixed',
                fixed = { -1 / 2, -1 / 2, -1 / 2, 1 / 2, 1 / 2 - 1 / 16, 1 / 2 },
            },
            selection_box = {
                type = 'fixed',
                fixed = { -1 / 2, -1 / 2, -1 / 2, 1 / 2, 1 / 2 - 1 / 16, 1 / 2 },
            },
            drop = def.drop or 'default:dirt',
            groups = { no_silktouch = 1, crumbly = 3, not_in_creative_inventory = 1 },
            sounds = def.sounds or default.node_sound_dirt_defaults(),
            mod_origin = def.mod_origin
        })
    end
end
