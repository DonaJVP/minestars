-- Minetest 0.4 mod: bones
-- See README.txt for licensing and other information.

local storage = core.get_mod_storage("bones")

local function is_owner(pos, name)
	local owner = minetest.get_meta(pos):get_string("owner")
	if owner == "" or owner == name or minetest.check_player_privs(name, "protection_bypass") then
		return true
	end
	return false
end
--[[
local bones_formspec =
	"size[8,9]" ..
	default.gui_bg ..
	default.gui_bg_img ..
	default.gui_slots ..
	"list[current_name;main;0,0.3;8,4;]" ..
	"list[current_player;main;0,4.85;8,1;]" ..
	"list[current_player;main;0,6.08;8,3;8]" ..
	"listring[current_name;main]" ..
	"listring[current_player;main]" ..
	default.get_hotbar_bg(0,4.85)
--]]

local function get_formspec(player, id)
	return "size[17,10]" ..
		"position[0.5,0.5]" ..
		default.gui_bg ..
		default.gui_bg_img ..
		default.gui_slots ..
		--
		"label[3.3,0;Bones]" ..
		"list[detached:BONES_" .. id .. ";main;0,5.5;8,4;]" ..
		"label[3.8,1.5;Crafting]" ..
		"list[detached:BONES_" .. id .. ";craft;3,2;3,3;]" ..
		"label[0.4,1.5;Armor]" ..
		"list[detached:BONES_" .. id .. ";armor;0,2;2,3;]" ..
		"label[6.25,3.5;Offhand]" ..
		"list[detached:BONES_" .. id .. ";offhand;6.5,4;1,1;]" ..
		--
		"label[12,0;Inventory]" ..
		"list[current_player;main;9,5.5;8,4;]" ..
		"label[9.4,1.5;Armor]" ..
		"list[detached:"..player:get_player_name().."_armor;armor;9,2;2,3;]" ..
		"label[12.8,1.5;Crafting]" ..
		"list[current_player;craft;12,2;3,3;]" ..
		"label[15.25,3.5;Offhand]" ..
		"list[detached:"..player:get_player_name().."_offhand;offhand;15.5,4;1,1;]" ..
		
		"image_button[0,9.6;17,0.7;blank.png;all;Get all]" ..
		default.get_hotbar_bg(0,4.85)
end

local share_bones_time = tonumber(minetest.settings:get("share_bones_time")) or 1200
local share_bones_time_early = tonumber(minetest.settings:get("share_bones_time_early")) or share_bones_time / 4

local bones_pos = {}

--just a coke for me please..... its so hot over here!

--Helpers
PassToItemStacks = function(list)
	local i_ = {}
	for _, itemraw in pairs(list) do
		i_[_] = ItemStack(itemraw)
	end
	return i_
end

-- GETnWORLDnTIME - BONESnTIME

--EIM [Entity Inventory Management]
local deim = {} --time and  player data
local eim = {}
deim.is_queryable = function(id, player)
	local meta = player:get_meta()
	local deadids = core.deserialize(meta:get_string("deadids")) or {}
	if deadids[id] then
		return true
	else
		local invdata = core.deserialize(storage:get_string("BONESDATA_"..id)) or {timecreated = core.get_gametime(), player = ""}--{timecreated=1101010, player=Jose}
		if invdata.timecreated - core.get_gametime() >= 500 then
			return true
		end
	end
	
	return false
end
eim.get_inventory_id = function(inv)
	local loc = inv:get_location()
	if loc then
		local name = loc.name
		if name then
			return name:split("_")[2] -- BONES_a1b2c3d then "a1b2c3d"
		end
	end
end
eim.update_inventory = function(inv, id)
	local id = id or eim.get_inventory_id(inv)
	if id then
		local __ = {}
		for _, typ in pairs({"main", "offhand", "craft", "armor"}) do
			__[typ] = {}
			for i, stack in pairs(inv:get_list(typ)) do
				table.insert(__[typ], stack:to_string())
			end
		end
		storage:set_string("BONES_"..id, core.serialize(__))
	end
end
eim.query_inventory = function(id)
	local inv_from_storage = core.deserialize(storage:get_string("BONES_"..id))
	--print(dump(inv_from_storage))
	if inv_from_storage then
		local was_new = core.get_inventory({type = "detached", name = "BONES_"..id}) == nil
		local new_inv = core.get_inventory({type = "detached", name = "BONES_"..id}) or core.create_detached_inventory("BONES_"..id, {
			allow_take = function(inv, listname, index, stack, player)
				local location = inv:get_location() --origin
				--print(dump(location))
				local inv_name = location and location.name
				if inv_name then
					local id = inv_name:split("_")[2]
					--print("COMMENCE HERE")
					--print(dump(id), deim.is_queryable(id, player))
					if deim.is_queryable(id, player) then
						return stack:get_count()
					end
				end
				return 0
			end,
			allow_put = function()
				return 0
			end,
			on_take = function(inv)
				eim.update_inventory(inv)
			end,
		})
		if was_new then
			new_inv:set_size("armor", 6)
			new_inv:set_size("main", 32)
			new_inv:set_size("offhand", 1)
			new_inv:set_size("craft", 9)
			for typp, invcontent in pairs(inv_from_storage) do
				invcontent = PassToItemStacks(invcontent)
				new_inv:set_list(typp, invcontent)
			end
		end
		return new_inv
	else
		core.log("error", "Unknown inventory with id '"..id.."'")
	end
end
eim.create_inv = function(id)
	local inv = core.create_detached_inventory("BONES_"..id, {
		allow_take = function(inv, listname, index, stack, player)
			local location = inv:get_location() --origin
			local inv_name = location and location.name
			if inv_name then
				local id = inv_name:split("_")[2]
				if deim.is_queryable(id, player) then
					return stack:get_count()
				end
			end
			return 0
		end,
		allow_put = function()
			return 0
		end,
		on_take = function(inv)
			eim.update_inventory(inv)
		end,
	})
	return inv
end


core.register_entity("bones:bones_ent", {
	initial_properties = {
		hp_max = 1,
		visual = "mesh",
		mesh = "skinsdb_3d_armor_character_5.b3d",
		textures = {"character.png", "blank.png", "blank.png", "blank.png"},
		visual_size = vector.new(1,1,1),
		static_save = true,
		physical = true,
		collisionbox = {-0.3, 0.0, -0.6, 0.3, 0.3, 0.6},
		selectionbox = {-0.3, 0.0, -0.6, 0.3, 0.3, 0.6},
	},
	--static data
	ID = "",
	TEXTURES__ = "character.png;blank.png;blank.png;blank.png",
	--detached inventory helps
	--[[inventory = {
		main = {},
		offhand = {},
		armor = {},
		craft = {},
	}--]]
	--FUnctions
	on_activate = function(self, data)
		--print("IID: "..data)
		local datatable = data:split(",")
		local id = datatable[1]
		local textures = datatable and datatable[2] and datatable[2]:split(";")
		if id then
			self.ID = id
			self.TEXTURES__ = datatable[2],
			self.object:set_armor_groups({immortal = 1})
			self.object:set_acceleration(vector.new(0,-9.81,0))
			self.object:set_properties({
				textures = textures,
			})
			self.object:set_animation({x=162, y=166})
		else
			self.object:remove()
		end
	end,
	get_staticdata = function(self)
		if self.ID and self.TEXTURES__ then
			return self.ID..","..self.TEXTURES__
		elseif self.ID then
			return self.ID..",character.png;blank.png;blank.png;blank.png"
		end
		--print("NULL")
	end,
	on_punch = function(self, player, time_from_last_punch, tool_capabilities, dir)
		--print("doom")
		if deim.is_queryable(self.ID, player) then
			local inv = eim.query_inventory(self.ID)
			if inv then
				--variables
				local can_delete = true
				--pinv
				local pinv = player:get_inventory()
				--local inventories
				for _, listname in pairs({"craft", "main", "offhand"}) do
					for i, stack in pairs(inv:get_list(listname)) do
						if pinv:room_for_item(listname, stack) then
							pinv:add_item(listname, stack)
							if listname ~= "offhand" then -- next lines
								inv:set_stack(listname, i, nil)
							end
						else
							can_delete = false
							break
						end
					end
				end
				--extended inventories [offhand]
				if can_delete then
					if inv:get_stack("offhand", 1):get_count() > 0 then
						--return_inv_or_create(player):set_list("offhand", inv:get_list())
						local inv__ = return_inv_or_create(player)
						if inv__ then
							if inv__:room_for_item("offhand", inv:get_stack("offhand", 1)) then
								inv__:add_item("offhand", inv:get_stack("offhand", 1))
								inv:set_stack("offhand", 1, nil)
							else
								can_delete = false
							end
						end
					end
				end
				if can_delete then
					local inv__ = core.get_inventory({type = "detached", name = player:get_player_name().."_armor"})
					local errors = 0 --if more than 1 then no delete the body
					for i, stack in pairs(inv:get_list("armor")) do
						if stack:get_count() > 0 then
							local can_add = true
							-- Make valid definitions
							local valid__ = {
								armor_torso = 0,
								armor_legs = 0,
								armor_feet = 0,
								armor_head = 0,
								armor_shield = 0,
							}
							local getted_value = ""
							-- Check armor groups of inventory (dead)
							local d_stack = stack:get_definition()
							for armortype, value in pairs(valid__) do
								if d_stack.groups[armortype] then
									valid__[armortype] = 1
									getted_value = armortype
									break
								end
							end
							-- Check armor groups of inventory in player
							-- search it
							local i = 0
							for _, armorstack in pairs(inv__:get_list("armor")) do
								local i = _
								if armorstack:get_count() > 0 then
									local def = armorstack:get_definition()
									if def.groups[getted_value] then
										errors = errors + 1
										can_add = false
										break
									end
								end
							end
							--query it
							if can_add then
								-- Assign shield
								if getted_value == "armor_shield" then
									MineStars.Shield.ShieldsHAS[Name(player)] = true
								end
								inv__:add_item("armor", stack)
							end
						end
					end
					can_delete = errors == 0
					armor:save_armor_inventory(player)
					armor:set_player_armor(player)
					armor:update_player_visuals(player)
				end
				if can_delete then
					storage:set_string("BONES_"..self.ID, "")
					storage:set_string("BONESDATA_"..self.ID, "")
					self.object:remove()
				end
			end
		end
	end,
	on_rightclick = function(self, clicker)
		if self.ID then
			--print(self.ID)
			eim.query_inventory(self.ID) --update inventory
			core.show_formspec(clicker:get_player_name(), "BONES:BONES", get_formspec(clicker, self.ID))
		end
	end
})


core.register_node("bones:bones", {
	description = "Bones",
	tiles = {
		"bones_top.png^[transform2",
		"bones_bottom.png",
		"bones_side.png",
		"bones_side.png",
		"bones_rear.png",
		"bones_front.png"
	},
	
	--[[visual = "mesh",
	visual_size = vector.new(1,1,1),
	mesh = "character_laying.obj",--]]
	
	paramtype2 = "facedir",
	groups = {dig_immediate = 2},
	sounds = default.node_sound_gravel_defaults(),

	
})

local function may_replace(pos, player)
	local node_name = minetest.get_node(pos).name
	local node_definition = minetest.registered_nodes[node_name]

	-- if the node is unknown, we return false
	if not node_definition then
		return false
	end

	-- allow replacing air and liquids
	if node_name == "air" or node_definition.liquidtype ~= "none" then
		return true
	end

	-- don't replace filled chests and other nodes that don't allow it
	local can_dig_func = node_definition.can_dig
	if can_dig_func and not can_dig_func(pos, player) then
		return false
	end

	-- default to each nodes buildable_to; if a placed block would replace it, why shouldn't bones?
	-- flowers being squished by bones are more realistical than a squished stone, too
	-- exception are of course any protected buildable_to
	return node_definition.buildable_to and not minetest.is_protected(pos, player:get_player_name())
end

local drop = function(pos, itemstack)
	local obj = minetest.add_item(pos, itemstack:take_item(itemstack:get_count()))
	if obj then
		obj:setvelocity({
			x = math.random(-10, 10) / 9,
			y = 5,
			z = math.random(-10, 10) / 9,
		})
	end
end

minetest.register_on_dieplayer(function(player)
	local bones_mode = "bones"
	local player_inv = player:get_inventory()
	if player_inv:is_empty("main") and player_inv:is_empty("craft") and player_inv:is_empty("offhand") then
		return
	end
	--[[local pos = vector.round(player:get_pos())
	local player_name = player:get_player_name()
	if bones_mode == "bones" and not may_replace(pos, player) then
		local air = minetest.find_node_near(pos, 1, {"air"})
		if air and not minetest.is_protected(air, player_name) then
			pos = air
		else
			bones_mode = "drop"
		end
	end
	if bones_mode == "drop" then
		-- drop inventory items
		for i = 1, player_inv:get_size("main") do
			drop(pos, player_inv:get_stack("main", i))
		end
		player_inv:set_list("main", {})
		-- drop crafting grid items
		for i = 1, player_inv:get_size("craft") do
			drop(pos, player_inv:get_stack("craft", i))
		end
		player_inv:set_list("craft", {})

		drop(pos, ItemStack("bones:bones"))
		return
	end--]]

	--local param2 = minetest.dir_to_facedir(player:get_look_dir())
	--minetest.set_node(pos, {name = "bones:bones", param2 = param2})

	local str = ""
	for _, text in pairs(player:get_properties().textures) do
		str = str..";"..text
		--print(str)
	end

	local tt__ = core.deserialize(player:get_meta():get_string("deadids")) or {}
	local id = FormRandomString(5)
	local obj = core.add_entity(player:get_pos(), "bones:bones_ent", id..","..str)
	if obj then
		local ent = obj:get_luaentity()
			ent.ID = id
		local inv = eim.create_inv(id)

		tt__[id] = true

		storage:set_string("BONESDATA_"..id, core.serialize({timecreated = core.get_gametime(), player = player:get_player_name()}))

		inv:set_size("main", 32)
		inv:set_size("offhand", 1)
		inv:set_size("armor", 6)
		inv:set_size("craft", 9)
		
		inv:set_list("craft", player_inv:get_list("craft"))
		inv:set_list("main", player_inv:get_list("main"))
		
		for _, inv_name in pairs({"craft", "main"}) do
			local list = player_inv:get_list(inv_name)
			inv:set_list(inv_name, list)
		end
		
		inv:set_list("offhand", return_inv_or_create(player):get_list("offhand"))
		
		player_inv:set_list("main", {})
		player_inv:set_list("offhand", {})
		player_inv:set_list("craft", {})
		return_inv_or_create(player):set_list("offhand", {})
		local inv__ = core.get_inventory({type = "detached", name = player:get_player_name().."_armor"})
		if inv__ then
			inv:set_list("armor", inv__:get_list("armor"))
			inv__:set_list("armor", {})
			armor:save_armor_inventory(player)
			armor:set_player_armor(player)
			armor:update_player_visuals(player)
		end
		
		eim.update_inventory(inv, id)

		--meta:set_string("formspec", bones_formspec)
		--meta:set_string("owner", player_name)
	--[[
		if share_bones_time ~= 0 then
			meta:set_string("infotext", player_name .. "'s fresh bones")

			if share_bones_time_early == 0 or not minetest.is_protected(pos, player_name) then
				meta:set_int("time", 0)
			else
				meta:set_int("time", (share_bones_time - share_bones_time_early))
			end

			minetest.get_node_timer(pos):start(10)
		else
			meta:set_string("infotext", player_name.."'s bones")
		end-]]
		
		--ent.ID = id
		
		player:get_meta():set_string("deadids", core.serialize(tt__))
		
		obj:set_properties({textures = player:get_properties().textures})
		obj:set_yaw(player:get_look_yaw())
		
		
		
		ent.TEXTURES__ = str
	else
		core.log("error", "Possibly lost frame!!!!! Dropping items.....")
		
	end
	player:set_properties({
		visual_size = vector.new(0,0,0)
	})
end)


core.register_on_respawnplayer(function(player)
	player:set_properties({
		visual_size = vector.new(1,1,1)
	})
	armor:update_player_visuals(player)
end)
