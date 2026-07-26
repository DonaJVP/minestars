-- Maximum number of light nodes above beacon (or 0 for no max)
--[[

local beacon_height = tonumber(minetest.settings:get("beacon_height")) or 0

local function get_node_under_light(pos)
    pos = table.copy(pos)
    local node = minetest.get_node(pos)
    local y_dist = 0
    while minetest.get_item_group(node.name, "beacon_light") ~= 0 and (beacon_height == 0 or y_dist <= beacon_height) do
        node = minetest.get_node(vector.subtract(pos, {x = 0, y = y_dist, z = 0}))
        y_dist = y_dist + 1
    end
    return node
end

local function beacon_on_construct(pos, color)
    local above = vector.add(pos, {x = 0, y = 1, z = 0})
    if get_node_under_light(pos).name == ("beacons:base_" .. color) and minetest.get_node(above).name == "air" then
        minetest.set_node(above, {name = "beacons:light_" .. color})
    end
end

local function beacon_on_destruct(pos, color)
    local above = vector.add(pos, {x = 0, y = 1, z = 0})
    if minetest.get_node(above).name == ("beacons:light_" .. color) then
        minetest.remove_node(above)
    end
end

local function beacon_on_timer(pos, color)
    beacon_on_construct(pos, color)
    if get_node_under_light(pos).name ~= ("beacons:base_" .. color) then
        minetest.remove_node(pos)
    end
    return true
end

local function make_on_construct(color)
    return function(pos)
        minetest.get_node_timer(pos):start(1)
        return beacon_on_construct(pos, color)
    end
end

local function make_on_destruct(color)
    return function(pos) 
        return beacon_on_destruct(pos, color)
    end
end

local function make_on_timer(color)
    return function(pos) 
        return beacon_on_timer(pos, color)
    end
end

local color_descs = {
    white = "White",
    red = "Red",
    yellow = "Yellow",
    green = "Green",
    cyan = "Cyan",
    blue = "Blue",
    magenta = "Magenta",
    orange = "Orange",
    violet = "Violet",
}

for color, color_desc in pairs(color_descs) do
    minetest.register_node("beacons:base_" .. color, {
        description = color_desc .. " Beacon",
        tiles = {"beacons_base_top.png", "beacons_base_bottom.png", "beacons_base_side.png"},
        color = color:gsub("_", ""),
        groups = {beacon_base = 1, cracky = 2},
        is_ground_content = false,
        paramtype = "light",
        on_rotate = function() end,
        
        on_construct = make_on_construct(color),
        on_destruct = make_on_destruct(color),
        on_timer = make_on_timer(color),
    })
    
    if minetest.get_modpath("default") then
        minetest.override_item("beacons:base_" .. color, {sounds = default.node_sound_metal_defaults()})
        
        if minetest.get_modpath("dye") then
            local color_group
            if color == "magenta" then
                color_group = "excolor_red_violet"
            else
                color_group = "excolor_" .. color
            end
            
            minetest.register_craft({
                output = "beacons:base_" .. color,
                recipe = {
                    {"default:steel_ingot", "group:" .. color_group, "default:steel_ingot",},
                    {"default:steel_ingot",      "default:meselamp", "default:steel_ingot",},
                    {"default:steel_ingot",   "default:steel_ingot", "default:steel_ingot",},
                },
            })
            
            minetest.register_craft({
                type = "shapeless",
                output = "beacons:base_" .. color,
                recipe = {"group:beacon_base", "group:" .. color_group},
            })
        end
    end
    
    minetest.register_node("beacons:light_" .. color, {
        description = color_desc .. " Beacon Light",
        drawtype = "plantlike",
        tiles = {"beacons_light.png"},
        color = color:gsub("_", ""),
        paramtype = "light",
        sunlight_propagates = true,
        light_source = minetest.LIGHT_MAX,
        walkable = false,
        pointable = false,
        diggable = false,
        buildable_to = true,
        floodable = false,
        drop = "",
        groups = {beacon_light = 1, not_in_creative_inventory = 1},
        damage_per_second = 1,
        post_effect_color = color,
        on_blast = function() end,
        
        on_construct = make_on_construct(color),
        on_destruct = make_on_destruct(color),
        on_timer = make_on_timer(color),
    })
end--]]

--MINESTARS CODE

local function on_place_node(pos, color)
	if pos and color then
		local lengh = 1 --plus one
		for i = pos.y+1, 31000 do
			local pos = vector.new(pos.x, i, pos.z)
			local posnode = core.get_node(pos)
			if posnode and ((posnode.name or "air") == "air") or posnode.name == "ignore" then
				core.set_node(pos, {name = "beacons:beacon_light_"..color});
			else
				break; --should break when found other node than "ignore" or other.....
			end
		end
	else
		core.log("error", "Error at placing nodes beacon: "..tostring(core.pos_to_string(pos))..": "..tostring(color))
	end
end
local function return_function_on_place(color)
	return function(pos)
		on_place_node(pos, color)
	end
end

local function on_destruct_node(pos)
	--Scan pos
	local blocks_lengh = 1 --default
	for i = pos.y+1, 31000 do --pos to limit
		local node = core.get_node(vector.new(pos.x, i, pos.z))
		blocks_lengh = i
		if node and not ((node.name or ""):match("beacon")) then
			break; -- lengh is already saved!
		end
	end
	--Proceed to delete all blocks
	for i = pos.y+1, blocks_lengh do
		core.set_node(vector.new(pos.x, i, pos.z), {name="air"})
	end
end

local color_descs = {
	white = "White",
	red = "Red",
	yellow = "Yellow",
	green = "Green",
	cyan = "Cyan",
	blue = "Blue",
	magenta = "Magenta",
	orange = "Orange",
	violet = "Violet",
}

do
	for color_str, desc in pairs(color_descs) do
		core.register_node("beacons:beacon_"..color_str, {
			description = desc.." Beacon",
			tiles = {"beacons_base_top.png", "beacons_base_bottom.png", "beacons_base_side.png"},
			color = color_str:gsub("_", ""),
			groups = {beacon_base = 1, cracky = 2},
			is_ground_content = false,
			paramtype = "light",
			on_rotate = function() end,
			on_construct = return_function_on_place(color_str),
			on_destruct = on_destruct_node,
			sounds = default.node_sound_metal_defaults(),
		})
		core.register_alias("beacons:base_"..color_str, "beacons:beacon_"..color_str)
		local color_group
		if color_str == "magenta" then
			color_group = "excolor_red_violet"
		else
			color_group = "excolor_" .. color_str
		end
		core.register_craft({
			output = "beacons:beacon_" .. color_str,
			recipe = {
				{"default:steel_ingot", "group:" .. color_group, "default:steel_ingot",},
				{"default:steel_ingot", "default:meselamp", "default:steel_ingot",},
				{"default:steel_ingot", "default:steel_ingot", "default:steel_ingot",},
			}
		})
		core.register_craft({
			type = "shapeless",
			output = "beacons:beacon_" .. color_str,
			recipe = {"group:beacon_base", "group:" .. color_group},
		})
		core.register_node("beacons:beacon_light_" .. color_str, {
			description = desc .. " Beacon Light",
			drawtype = "plantlike",
			tiles = {"beacons_light.png"},
			color = color_str:gsub("_", ""),
			paramtype = "light",
			sunlight_propagates = true,
			light_source = core.LIGHT_MAX,
			walkable = false,
			pointable = false,
			diggable = false,
			buildable_to = true,
			floodable = false,
			drop = "",
			groups = {beacon_light = 1, not_in_creative_inventory = 1},
			damage_per_second = 1,
			post_effect_color = color_str,
			on_blast = function() end,
		})
		core.log("action", "Registered beacom \""..desc.."\"")
	end
end




