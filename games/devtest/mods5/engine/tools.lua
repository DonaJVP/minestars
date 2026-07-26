--Void Tool
--API
local function after(ref)
	core.after(3, function(ref)
		if ref and ref:get_yaw() then
			ref:set_velocity(vector.new())
		end
	end, ref)
end
--Consists that the first_player can throw the other player [IF POINTING] to the direction multiplied by 4
core.register_tool("engine:void_kicker", {
	description = MineStars.Translator("Make impulse to a obj that you see"),
	light_source = 9,
	inventory_image = "void_kicker.png",
	on_use = function(itemstack, user, pointed_thing)
		if pointed_thing and pointed_thing.type == "object" then
			if user then
				--7955
				if pointed_thing.ref:is_player() then
					local dir = vector.multiply(vector.direction(vector.add(user:get_pos(), vector.new(0, 1.47, 0)), pointed_thing.ref:get_pos()), 18)
					pointed_thing.ref:add_player_velocity(dir)
				else
					local dir = vector.multiply(vector.direction(vector.add(user:get_pos(), vector.new(0, 1.47, 0)), pointed_thing.ref:get_pos()), 18)--vector.multiply(vector.subtract(pointed_thing.ref:get_pos(), vector.add(user:get_pos(), vector.new(0, 1.47, 0))), 4)-- get_pos
					pointed_thing.ref:set_velocity(dir)
					after(pointed_thing.ref)
				end
				itemstack:set_wear(itemstack:get_wear() + 2000)
			end
		else -- impulse player
			local dir = vector.multiply(user:get_look_dir(), 18)
			user:add_player_velocity(dir)
			itemstack:set_wear(itemstack:get_wear() + 2000)
		end
		
		return itemstack
	end,
	on_secondary_use = function(i, u)
		i:set_wear(i:get_wear() + 2000)
		local dir = vector.multiply(u:get_look_dir(), 18)
		u:add_player_velocity(dir)
	end,
	sound = {breaks = "default_tool_breaks"},
})
--Void teleporter (engine:teleport_item[HIDDEN] engine:teleport_raw[SHOWN])
--core.register_tool("engine:void_fly")
local function teleport(i, u)
	local meta = i:get_meta()
	if meta then
		local pos_str = meta:get_string("pos")
		if pos_str and pos_str ~= "" then
			local pos = core.string_to_pos(pos_str)
			if pos then
				u:set_pos(CheckPos(pos))
				i:set_wear(i:get_wear() + 4000)
				return i
			end
		end
	end
end

local function teleport2(i,u,p)
	if p then
		local pos = p.above 
		if pos then
			local meta = i:get_meta()
			if meta then
				meta:set_string("pos", core.pos_to_string(pos))
				i:set_name("engine:teleport_item")
				return i
			end
		end
	else
		--get player pos and check
		local pos = CheckPos(u:get_pos())
		if pos then
			local meta = i:get_meta()
			if meta then
				meta:set_string("pos", core.pos_to_string(pos))
				i:set_name("engine:teleport_item")
				return i
			end
		end
	end
end

core.register_tool("engine:teleport_item", {
	description = MineStars.Translate("Teleport item, use to teleport where you set a place"),
	inventory_image = "teleport_item.png",
	groups = {
		not_in_creative_inventory = 1,
	},
	on_use = teleport,
	on_secondary_use = teleport,
})

core.register_tool("engine:teleport_raw", {
	description = MineStars.Translate("Teleport item, punch a place to set pos for teleport item"),
	inventory_image = "teleport_item_raw.png",
	on_use = teleport2,
	on_secondary_use = teleport2,
})

--Nickel Compass
--Make the player see some materials underground with the radious of 7! 
--Obj
core.register_entity("engine:scan_obj", {
	initial_properties = {
		visual = "sprite",
		textures = {"blank.png"},
		visual_scale = vector.new(),
		collisionbox = {0,0,0,0,0,0},
		physical = false,
		nametag = "",
		static_save = false,
	},
	timer = 5,
	typoname = "",
	on_step = function(self, dtime)
		self.timer = self.timer - dtime
		if self.timer <= 0 then
			self.object:remove()
		else
			self.object:set_properties({
				nametag = (self.typoname or "Ore!").."\n"..math.round(self.timer).."s",
			})
		end
	end
})
--Func
local function scan_around_and_set_obj(pos)
	local area1, area2 = RadiusToArea(pos, 7)
	for _, pos in pairs(core.find_nodes_in_area(area1, area2, {"group:ore"})) do
		local node = core.get_node(pos)
		if node and node.name then
			if core.registered_items[node.name] and math.random(1, 40) > 30 then --chance
				local obj = core.add_entity(pos, "engine:scan_obj")
				local ent = obj:get_luaentity()
				if ent then
					ent.typoname = ItemStack(node.name):get_short_description()
				end
			end
		end
	end
end

--Item/Wand

core.register_tool("engine:nickel_compass", {
	inventory_image = "nickel_compass.png",
	description = MineStars.Translator("Nickel Compass").."\n"..MineStars.Translator("Use this tool to see hidden ores around you"),
	on_use = function(itemstack, user, pointed_thing)
		if user and itemstack then
			local pos = vector.apply(user:get_pos(), math.round)
			scan_around_and_set_obj(pos)
		end
		itemstack:set_wear(itemstack:get_wear() + 4000)
		return itemstack
	end,
	sound = {breaks = "default_tool_breaks"},
})

--Mixing Sunstone and Elementium generates a rare magic effects, making some special things

core.register_craftitem("engine:elestone_ingot", {
	description = "Elestone",
	inventory_image = "elestone.png",
})

--Elestone Axe 
--spec. func.
--[[local positions_ = {
	vector.new(1, 0, 0),
	vector.new(0, 0, 1),
	vector.new(0, 1, 0),
	vector.new(1, 0, 1),
	vector.new(1, 1, 0),
	vector.new(1, 1, 1),
	vector.new(0, 1, 1),
	--vector.new(0, 0, 0)
}
local function dig_n_see_other(pos, nodename, positions)
	local nodest = core.get_node(pos)
	if nodest.name == nodename then
		core.add_item(pos, core.registered_items[nodename].drop or nodename)
		core.set_node(pos, {name = "air"})
	end
	for _, poss in pairs(positions) do
		local pos_ = vector.add(pos, poss)
		local node = core.get_node(pos_)
		if node.name == nodename then
			local drops = core.registered_items[nodename].drop or nodename
			core.add_item(pos_, drops)
			core.set_node(pos_, {name = "air"})
			--dig_n_see_other(pos_, nodename) 
		else
			local positions__ = table.copy(positions_)
			positions[_] = nil
			dig_n_see_other(pos_, nodename, positions)
		end
		--dig_n_see_other(pos_, nodename)
	end
end--]]
local function dig_n_see_other(pos, nodename)
	for y = math.min(pos.y, 65535), math.max(pos.y+20,pos.y) do
		local pos_ = vector.new(pos.x, y, pos.z)
		local node = core.get_node(pos_)
		if node.name == nodename then
			core.add_item(pos_, nodename)
			core.set_node(pos_, {name = "air"})
		end
	end 
end


core.register_tool("engine:elestone_axe", {
	description = MineStars.Translator"Elestone Axe",
	inventory_image = "elestone_axe.png",
	tool_capabilities = {
		full_punch_interval = 0.1,
		max_drop_level=3,
		groupcaps={
			choppy={times={[1]=0.50, [2]=0.2, [3]=0.05}, uses=350, maxlevel=3},
		},
		damage_groups = {fleshy=9},
	},
	on_place = function(itemstack, user, pointed_thing)
		if pointed_thing.under and core.get_node(pointed_thing.under) and core.get_node(pointed_thing.under).name and core.registered_items[core.get_node(pointed_thing.under).name].groups and core.registered_items[core.get_node(pointed_thing.under).name].groups.tree and core.registered_items[core.get_node(pointed_thing.under).name].groups.tree >= 1 then
			dig_n_see_other(pointed_thing.under, core.get_node(pointed_thing.under).name, positions_)
		end
		itemstack:set_wear(itemstack:get_wear() + 2000)
		return itemstack
	end,
	sound = {breaks = "default_tool_breaks"},
})

local function on_use_br(itm, player)
	player:set_hp(((player:get_hp() + 5) <= 10 and (player:get_hp() + 5)) or 20)
	itm:set_wear(itm:get_wear() + 10000)
	return itm
end

core.register_tool("engine:bloodstone_rock", {
	description = MineStars.Translator("Bloodstone healing rock"),
	inventory_image = "bloodstone_rock.png",
	on_use = on_use_br,
	on_secondary_use = on_use_br,
})

core.register_tool("engine:emerald_staff", {
	description = MineStars.Translator("Emerald Staff, use this to heal other players!"),
	inventory_image = "emerald_staff.png",
	on_use = function(itm, user, pt)
		if user and pt.ref then
			local max_hp = (pt.ref:is_player() and 20) or (pt.ref:get_properties() and pt.ref:get_properties().hp_max) 
			local hp = pt.ref:get_hp()
			local sum = ((hp + 4) <= max_hp and (hp + 4)) or max_hp
			pt.ref:set_hp(sum)
			itm:set_wear(itm:get_wear() + 9000) 
		else
			local sum = ((user:get_hp() + 4) <= 20 and (user:get_hp() + 4)) or 20
			user:set_hp(sum)
			itm:set_wear(itm:get_wear() + 9000)
		end
		return itm
	end
})

core.register_tool("engine:onyx_dagger", {
	description = MineStars.Translator("Onyx Dagger"),
	inventory_image = "onyx_dagger.png",
	tool_capabilities = {
		full_punch_interval = 0.1,
		max_drop_level=1,
		groupcaps={
			snappy={times={[1]=3, [2]=2, [3]=1}, uses=20, maxlevel=3},
		},
		damage_groups = {fleshy=9},
	},
})
--[[
core.register_craftitem("engine:", {
	description = MineStars.Translator(),
	inventory_image = "",
})
--]]

local function get_pointed_thing(player)
	if player:get_pos() then
		MineStars.Offhand.Cache[Name(player)] = MineStars.Offhand.GetHand(player)
		--- determine pointing range
		local range = 5
		-- adjust player position
		local pos1 = player:get_pos()
		pos1 = vector.add(pos1, player:get_eye_offset())
		pos1.y = pos1.y + (player:get_properties()).eye_height
		-- cast ray from player's eyes
		local pos2 = vector.add(pos1, vector.multiply(player:get_look_dir(), range))
		local ray = Raycast(pos1, pos2, false, false)
		-- iterate through passed nodes and determine if pointable
		local result = nil
		for pointed_thing in ray do
			if pointed_thing.type == "node" then
				local node = minetest.get_node(pointed_thing.under)
				local nodedef = minetest.registered_nodes[node.name]
				if nodedef and nodedef.pointable ~= false then
					result = pointed_thing
					break
				end
			else
				result = nil
				break
			end
		end
		return result
	else
		return false
	end
end

core.register_tool("engine:special_pickaxe", {
	description = MineStars.Translator("ELE Pickaxe"),
	inventory_image = "ele_pickaxe.png",
	tool_capabilities = {
		full_punch_interval = 0.1,
		max_drop_level=1,
		groupcaps={
			cracky={times={[1]=1, [2]=0.5, [3]=0.1}, uses=60, maxlevel=3},
		},
		damage_groups = {fleshy=9},
	},
	after_use = function(itemstack, user, node, digparams)
		local pointed_thing = get_pointed_thing(user)
		local inv = Inv(user)
		if pointed_thing and pointed_thing.under then
			local lk = vector.round(user:get_look_dir())
			local lkm = vector.round(vector.multiply(lk, 0.5))
			local pos = vector.round(vector.add(pointed_thing.under, lkm))
			--AREA
			local p1, p2 = RadiusToArea(pos, 1)
			--proccess map
			for y = math.min(p1.y, p2.y), math.max(p1.y, p2.y) do
				for z = math.min(p1.z, p2.z), math.max(p1.z, p2.z) do
					for x = math.min(p1.x, p2.x), math.max(p1.x, p2.x) do
						local node = core.get_node({x = x, y = y, z = z})
						if node --[[and node.name ~= "air"--]] then
							local drop = core.registered_items[node.name] or node.name
							inv:add_item("main", ItemStack(drop))
						end
						core.set_node({x = x, y = y, z = z}, {name = "air"})
					end
				end
			end
		end
		itemstack:set_wear(digparams.wear * 3)
		return itemstack
	end,
})







