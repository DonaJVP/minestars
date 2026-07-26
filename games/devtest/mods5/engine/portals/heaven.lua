core.register_node("engine:heaven_dirt", {
	description = "MCL Dirt",
	tiles = {"minecraft_dirt.png"},
	groups = {crumbly = 3, soil = 1},
	sounds = default.node_sound_dirt_defaults(),
})

core.register_node("engine:heaven_stone", {
	description = "MCL Stone",
	tiles = {"minecraft_stone.png"},
	groups = {cracky = 3, stone = 1},
	drop = "engine:heaven_cobble",
	sounds = default.node_sound_stone_defaults(),
})

minetest.register_node("engine:heaven_sand", {
	description = "Sand",
	tiles = {"minecraft_sand.png"},
	groups = {crumbly = 3, falling_node = 1, sand = 1},
	sounds = default.node_sound_sand_defaults(),
})

core.register_node("engine:heaven_cobble", {
	description = "MCL Cobblestone",
	tiles = {"minecraft_cobble.png"},
	groups = {cracky = 3, stone = 1},
	sounds = default.node_sound_stone_defaults(),
})

core.register_craft({
	type = "cooking",
	output = "engine:heaven_stone",
	recipe = "engine:heaven_cobble",
})

core.register_node("engine:heaven_grass", {
	description = "MCL Dirt with Grass",
	tiles = {"minecraft_grass.png", "minecraft_dirt.png", {name = "minecraft_dirt.png^minecraft_grass_side.png", tileable_vertical = false}},
	groups = {crumbly = 3, soil = 1, spreading_dirt_type = 1},
	drop = "engine:heaven_dirt",
	sounds = default.node_sound_dirt_defaults({
		footstep = {name = "default_grass_footstep", gain = 0.25},
	}),
})

minetest.register_node("engine:heaven_water", {
	description = "MCL Water Source",
	drawtype = "liquid",
	waving = 3,
	tiles = {
		{
			name = "minecraft_water_source_animated.png",
			backface_culling = false,
			animation = {
				type = "vertical_frames",
				aspect_w = 16,
				aspect_h = 16,
				length = 2.0,
			},
		},
		{
			name = "minecraft_water_source_animated.png",
			backface_culling = true,
			animation = {
				type = "vertical_frames",
				aspect_w = 16,
				aspect_h = 16,
				length = 2.0,
			},
		},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	is_ground_content = false,
	drop = "",
	drowning = 1,
	liquidtype = "source",
	liquid_alternative_flowing = "engine:heaven_water_flowing",
	liquid_alternative_source = "engine:heaven_water",
	liquid_viscosity = 1,
	post_effect_color = {a = 103, r = 30, g = 60, b = 90},
	groups = {water = 3, liquid = 3, cools_lava = 1},
	sounds = default.node_sound_water_defaults(),
})

minetest.register_node("engine:heaven_water_flowing", {
	description = "MCL Flowing Water",
	drawtype = "flowingliquid",
	waving = 3,
	tiles = {"minecraft_water.png"},
	special_tiles = {
		{
			name = "minecraft_water_flowing_animated.png",
			backface_culling = false,
			animation = {
				type = "vertical_frames",
				aspect_w = 16,
				aspect_h = 16,
				length = 0.5,
			},
		},
		{
			name = "minecraft_water_flowing_animated.png",
			backface_culling = true,
			animation = {
				type = "vertical_frames",
				aspect_w = 16,
				aspect_h = 16,
				length = 0.5,
			},
		},
	},
	use_texture_alpha = "blend",
	paramtype = "light",
	paramtype2 = "flowingliquid",
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	is_ground_content = false,
	drop = "",
	drowning = 1,
	liquidtype = "flowing",
	liquid_alternative_flowing = "engine:heaven_water_flowing",
	liquid_alternative_source = "engine:heaven_water",
	liquid_viscosity = 1,
	post_effect_color = {a = 103, r = 30, g = 60, b = 90},
	groups = {water = 3, liquid = 3, not_in_creative_inventory = 1,
		cools_lava = 1},
	sounds = default.node_sound_water_defaults(),
})

--defined in todofile.lua for C++ mapgen
--[[
local ores__ = table.copy(Dimensions.Definitions.Ores)
ores__["sunstone:sunstone_ore"] = 30000
ores__["moonstone:moonstone_ore"] = 30000

Dimensions.Register("heaven",{
	ground_ores = table.copy(Dimensions.Definitions.Plants),
	stone_ores = ores__,
	sand_ores = {["default:clay"] = {chunk=2,chance=5000}},
	grass_ores={
		["default:dirt_with_snow"]={chance=1,max_heat=20},
	},
	water_ores={
		["default:ice"]={chance=1,max_heat=20},
	},
	--Some nodes
	stone = "engine:heaven_stone",
	grass = "engine:heaven_grass",
	water = "engine:heaven_water",
	sand = "engine:heaven_sand",
	dirt = "engine:heaven_dirt",
})
--]]
local function hilbert_flip(cell_count, pos, flip_direction, flip_twice)
	if not flip_twice then
		if flip_direction then
			pos.x = (cell_count - 1) - pos.x;
			pos.y = (cell_count - 1) - pos.y;
		end

		local temp_x = pos.x;
		pos.x = pos.y;
		pos.y = temp_x;
	end
end

local function test_bit(cell_count, value, flag)
	local bit_value = cell_count / 2
	while bit_value > flag and bit_value >= 1  do
		if value >= bit_value then value = value - bit_value end
		bit_value = bit_value / 2
	end
	return value >= bit_value
end

-- Converts (x,y) to distance
-- starts at bottom left corner, i.e. (0, 0)
-- ends at bottom right corner, i.e. (cell_count - 1, 0)
local function get_hilbert_distance (cell_count, x, y)
	local distance = 0
	local pos = {x=x, y=y}
	local rx, ry

	local s = cell_count / 2
	while s > 0 do

		if test_bit(cell_count, pos.x, s) then rx = 1 else rx = 0 end
		if test_bit(cell_count, pos.y, s) then ry = 1 else ry = 0 end

		local rx_XOR_ry = rx
		if ry == 1 then rx_XOR_ry = 1 - rx_XOR_ry end -- XOR'd ry against rx

		distance = distance + s * s * (2 * rx + rx_XOR_ry)
		hilbert_flip(cell_count, pos, rx > 0, ry > 0);

		s = math.floor(s / 2)
	end
	return distance;
end

-- Converts distance to (x,y)
local function get_hilbert_coords(cell_count, distance)
	local pos = {x=0, y=0}
	local rx, ry

	local s = 1
	while s < cell_count do
		rx = math.floor(distance / 2) % 2
		ry = distance % 2
		if rx == 1 then ry = 1 - ry end -- XOR ry with rx

		hilbert_flip(s, pos, rx > 0, ry > 0);
		pos.x = pos.x + s * rx
		pos.y = pos.y + s * ry
		distance = math.floor(distance / 4)

		s = s * 2
	end
	return pos
end


local get_moore_distance = function(cell_count, x, y)

	local quadLength = cell_count / 2
	local quadrant = 1 - math.floor(y / quadLength)
	if math.floor(x / quadLength) == 1 then quadrant = 3 - quadrant end
	local flipDirection = x < quadLength

	local pos = {x = x % quadLength, y = y % quadLength}
	hilbert_flip(quadLength, pos, flipDirection, false)

	return (quadrant * quadLength * quadLength) + get_hilbert_distance(quadLength, pos.x, pos.y)
end


-- Converts distance to (x,y)
-- A Moore curve is a variation of the Hilbert curve that has the start and
-- end next to each other.
-- Top middle point is the start/end location
local get_moore_coords = function(cell_count, distance)
	local quadLength = cell_count / 2
	local quadDistance = quadLength * quadLength
	local quadrant = math.floor(distance / quadDistance)
	local flipDirection = distance * 2 < cell_count * cell_count
	local pos = get_hilbert_coords(quadLength, distance % quadDistance)
	hilbert_flip(quadLength, pos, flipDirection, false)

	if quadrant >= 2     then pos.x = pos.x + quadLength end
	if quadrant % 3 == 0 then pos.y = pos.y + quadLength end

	return pos
end

nether.register_portal("heaven_portal", {
	shape               = nether.PortalShape_Circular,
	frame_node_name     = "bloodstone:bloodstone_block",
	wormhole_node_name  = "nether:portal_alt",
	wormhole_node_color = 3,
	title = "Heaven Portal | God Temple",
	book_of_portals_pagetext = "This portal is the same as the surface portal, but built with the opal block",
	is_within_realm = function(pos)
		return pos.y >= 2500
	end,
	find_realm_anchorPos = function(realm_anchorPos, player_name)
		local existing_portal_location, existing_portal_orientation = nether.find_nearest_working_portal("heaven_portal", {x = realm_anchorPos.x, y = 2500, z = realm_anchorPos.z}, 80, 0)
		if existing_portal_location ~= nil then
			return existing_portal_location, existing_portal_orientation
		else
			-- UNSUPPORTED MULTICRAFT!
			--print(dump(realm_anchorPos))
			--return vector.new(math.random(realm_anchorPos.x * -2, realm_anchorPos.x * 2 + 1), math.random(2200,2400), math.random(realm_anchorPos.z * -2 + 1, realm_anchorPos.z * 2 + 1))
			return {
				x = realm_anchorPos.x,
				y = math.random(2400,2500),
				z = realm_anchorPos.z,
			}
		end
	end,
	on_player_teleported = function(def, player)
		player = Player(player)
		--print("got!")
		MineStars.PlayersAdvantage.OnTeleport(def.name__, player)
		return false --avoid other summoning sounds
	end,
	name__ = "heaven",
})

--MineStars.PlayersAdvantage.OnTeleport












