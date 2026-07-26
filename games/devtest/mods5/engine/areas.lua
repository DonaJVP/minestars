MineStars.Areas = {
	Radius = 10,
	Callbacks = {}
}
local S = MineStars.Translator

local function to_area(pos, r)
	return {x = pos.x - r, y = pos.y - r, z = pos.z - r}, {x = pos.x + r, y = pos.y + r, z = pos.z + r}
end

local T = function(_) if _ ~= {} then __ = {} for ___ in pairs(_) do table.insert(__, ___) end return __ else return {"Nothing..."} end end

local function formspecinfo(name, members, pname, error_, lvl, i)
	lvl=lvl or "1"
	return "size[4,3]"..
	"box[-0.3,-0.3;4.35,0.35;#009200]"..
	"label[0,-0.38;Protector x "..lvl.." | "..(name or "Unknown name").."]"..
	"label[-0.2,0.01;"..S("Members").."]"..
	"textlist[-0.2,0.4;2,2.3;textlist;"..table.concat(T(members), ",")..";"..(i or 1).."]"..
	"image_button[-0.21,2.8;2.3,0.7;blank.png;btn;Save]"..
	"image_button_exit[1.95,2.8;2.3,0.7;blank.png;btn_;Quit]"..
	"field[2.20,0.50;2.30,1.3;field_pyn;Player name;"..(pname or "").."]"..
	"image_button[1.92,1.2;2.32,0.6;blank.png;btn_kick;Kick member]".."image_button[1.92,1.7;2.32,0.6;blank.png;btn_add;Add member]".."label[1.92,2.2;"..(error_ or "").."]"
end

local function can_bypass(pname)
	return core.check_player_privs(pname, {protection_bypass = true})
end

function MineStars.Areas.CanModify(name, pos, plus)
	name=Name(name)
	--Values
	local _1, _2 = to_area(pos, MineStars.Areas.Radius*(plus or 1))
	local nodes_pos = core.find_nodes_in_area(_1, _2, {"engine:areas_security_1"})
	--Extra big protector (multiplied by 2)
	local _12, _22 = to_area(pos, (MineStars.Areas.Radius*2)*(plus or 1))
	local nodes_posBIG = core.find_nodes_in_area(_12, _22, {"engine:areas_security_2"})
	--Data load
	for _, pos in pairs(nodes_pos) do
		local meta = core.get_meta(pos)
		if meta then
			local owner = meta:get_string("area_owner")
			local members = core.deserialize(meta:get_string("members")) or {} -- members[name] != true
			if (owner == name) or members[name] or can_bypass(name) then
				return true
			else
				return false, owner
			end
		end
	end
	for _, pos in pairs(nodes_posBIG) do
		local meta = core.get_meta(pos)
		if meta then
			local owner = meta:get_string("area_owner")
			local members = core.deserialize(meta:get_string("members")) or {} -- members[name] != true
			if (owner == name) or members[name] or can_bypass(name) then
				return true
			else
				return false, owner
			end
		end
	end
	return true
end



function MineStars.Areas.SingleBlockCheck(name, pos, onlyowner)
	local meta = core.get_meta(pos)
	if meta then
		local owner = meta:get_string("area_owner")
		local members = core.deserialize(meta:get_string("members")) or {}
		if onlyowner then
			if owner == name then
				return true
			else
				return false
			end
			return false
		end
		if (owner == name) or members[name] then
			return true
		else
			return false
		end
	end
end

function MineStars.Areas.CanPlaceAnotherProtectorBlock(name, pos)
	local bool, nameofproperty = MineStars.Areas.CanModify(name, pos, 2)
	if not bool then
		core.chat_send_player(name, core.colorize("#FF8E8E", "✗"..MineStars.Translator("This area overlaps with the area of @1", nameofproperty)))
		return false
	end
	return bool
end

local oldie = core.is_protected
function core.is_protected(pos, pname)
	local player = Player(pname)
	if player then
		--First, check the principal protects, later, check other callbacks
		local bool, property = MineStars.Areas.CanModify(pname, pos, 1)
		if not bool then
			core.chat_send_player(pname, core.colorize("#FF8E8E", "✗"..MineStars.Translator("This area is owned by @1", property)))
			return true
		end
		--Callbacks
		local boolean, text = RunCallbacks(MineStars.Areas.Callbacks, player, pos)
		if boolean then
			if text then
				core.chat_send_player(pname, text)
			end
			return true
		end
	end
	return oldie(pos, pname)
end

--Ensure other mod is not compromised with this....
core.register_on_mods_loaded(function()
	core.after(0.5, function() -- security
		local old = core.is_protected
		core.is_protected = function(pos, pname)
			local player = Player(pname)
			if player then
				--First, check the principal protects, later, check other callbacks
				local bool, property = MineStars.Areas.CanModify(pname, pos, 1)
				if not bool then
					core.chat_send_player(pname, core.colorize("#FF8E8E", "✗"..MineStars.Translator("This area is owned by @1", property)))
					return true
				end
				--Callbacks
				local boolean, text = RunCallbacks(MineStars.Areas.Callbacks, player, pos)
				if boolean then
					if text then
						core.chat_send_player(pname, text)
					end
					return true
				end
			end
			return old(pos, pname)
		end
	end)
end)

--Callbacks

function MineStars.Areas.RegisterCheckFunc(func)
	table.insert(MineStars.Areas.Callbacks, func)
end

--variables

local cache_areas_poss = {}
local LVL = {}

--Nodes

core.register_node("engine:areas_security_1", {
	description = MineStars.Translator("Areas protector, level 1"),
	drawtype = "nodebox",
	tiles = {
		"circle_stone_block.png",
		"circle_stone_block.png",
		"circle_stone_block.png^__protector_logo.png"
	},
	groups = {dig_immediate = 2, unbreakable = 1},
	is_ground_content = false,
	paramtype = "light",
	light_source = 4,
	node_box = {
		type = "fixed",
		fixed = {
			{-0.5 ,-0.5, -0.5, 0.5, 0.5, 0.5},
		}
	},
	on_place = function(itemstack, placer, pointed_thing)
		if pointed_thing.type == "node" then
			local pos = pointed_thing.above
			local bool = MineStars.Areas.CanPlaceAnotherProtectorBlock(Name(placer), pos)
			if not bool then
				return itemstack
			end
			return core.item_place(itemstack, placer, pointed_thing)
		else
			return itemstack
		end
	end,
	after_place_node = function(pos, placer)
		local meta = core.get_meta(pos)
		meta:set_string("area_owner", placer:get_player_name() or "")
		meta:set_string("infotext", MineStars.Translator("Protection (owned by @1)", placer:get_player_name()))
		meta:set_string("members", "return {}")
	end,
	on_rightclick = function(pos, node, clicker, itemstack)
		local meta = core.get_meta(pos)
		local name = Name(clicker)
		if meta and MineStars.Areas.SingleBlockCheck(name, pos, true) then
			cache_areas_poss[name] = {pos=pos,members=core.deserialize(meta:get_string("members"))}
			core.show_formspec(name, "engine:areas_config", formspecinfo(Name(clicker), core.deserialize(meta:get_string("members")), "", "<No errors>", "1"))
			LVL[name] = 1
		end
	end,
	on_punch = function(pos, node, puncher)
		if core.is_protected(pos, Name(puncher)) then
			return
		end
		core.add_entity(pos, "engine:entities__display__security")
	end,
	can_dig = function(pos, player)
		return player and MineStars.Areas.SingleBlockCheck(Name(player), pos)
	end,
	on_blast = function() end,
})

core.register_node("engine:areas_security_2", {
	description = MineStars.Translator("Areas protector, level 2"),
	drawtype = "nodebox",
	tiles = {
		"circle_stone_block.png",
		"circle_stone_block.png",
		"circle_stone_block.png^__protector_logo_2.png"
	},
	sounds = nil,
	groups = {dig_immediate = 2, unbreakable = 1},
	is_ground_content = false,
	paramtype = "light",
	light_source = 4,
	node_box = {
		type = "fixed",
		fixed = {
			{-0.5 ,-0.5, -0.5, 0.5, 0.5, 0.5},
		}
	},
	on_place = function(itemstack, placer, pointed_thing)
		if pointed_thing.type == "node" then
			local pos = pointed_thing.above
			local bool = MineStars.Areas.CanPlaceAnotherProtectorBlock(Name(placer), pos)
			if not bool then
				return itemstack
			end
			return core.item_place(itemstack, placer, pointed_thing)
		else
			return itemstack
		end
	end,
	after_place_node = function(pos, placer)
		local meta = core.get_meta(pos)
		meta:set_string("area_owner", placer:get_player_name() or "")
		meta:set_string("infotext", MineStars.Translator("Protection (owned by @1)", placer:get_player_name()))
		meta:set_string("members", "return {}")
	end,
	on_rightclick = function(pos, node, clicker, itemstack)
		local meta = core.get_meta(pos)
		local name = Name(clicker)
		if meta and MineStars.Areas.SingleBlockCheck(name, pos, true) then
			cache_areas_poss[name] = {pos=pos,members=core.deserialize(meta:get_string("members"))}
			core.show_formspec(name, "engine:areas_config", formspecinfo(Name(clicker), cache_areas_poss[name].members, "", "<No errors>", "2"))
			LVL[name] = 2
		end
	end,
	on_punch = function(pos, node, puncher)
		if core.is_protected(pos, Name(puncher)) then
			return
		end
		core.add_entity(pos, "engine:entities__display__security_2")
	end,
	can_dig = function(pos, player)
		return player and MineStars.Areas.SingleBlockCheck(Name(player), pos)
	end,
	on_blast = function() end,
})

local selected_user_to_do_smth = {}

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "engine:areas_config" then
		if (not fields.quit) then
			local str_err = ""
			local I = 1
			local list = core.explode_textlist_event(fields.textlist)
			local POS_DATA = table.copy(cache_areas_poss[player:get_player_name()])
			local META = core.get_meta(POS_DATA.pos)
			local immediate_restart_form = false
			if fields.textlist then
				if list.type == "CHG" then
					if POS_DATA then
						local IDX__ = T(POS_DATA.members)
						local I_STR = IDX__[list.index]
						I = list.index
						selected_user_to_do_smth[player:get_player_name()] = I_STR
						immediate_restart_form = true
					else
						str_err = X.." !!!"
						immediate_restart_form = true
					end
				end
			end
			if fields.field_pyn then
				selected_user_to_do_smth[player:get_player_name()] = fields.field_pyn
				immediate_restart_form = true
			end
			if fields.btn then
				core.chat_send_player(Name(player), core.colorize("lightgreen", C_..S("Done!")))--#FF7C7C
			end
			if fields.btn_kick then
				if POS_DATA then
					if selected_user_to_do_smth[Name(player)] and selected_user_to_do_smth[Name(player)] ~= "" then
						if not POS_DATA.members[selected_user_to_do_smth[Name(player)]] then
							--core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..S("")))
							str_err = X..S("Not in members!")
							immediate_restart_form = true
						else
							POS_DATA.members[selected_user_to_do_smth[Name(player)]] = nil
							local dta = core.serialize(POS_DATA.members)
							META:set_string("members", dta)
						end
					else
						str_err = X..S("Invalid player")
						immediate_restart_form = true
					end
				else
					str_err = X.." !!!"
					immediate_restart_form = true
				end
			end
			if fields.btn_add then
				if POS_DATA then
					if selected_user_to_do_smth[Name(player)] and selected_user_to_do_smth[Name(player)] ~= "" then
						if POS_DATA.members[selected_user_to_do_smth[Name(player)]] then
							str_err = X..S("Already on members!")
							immediate_restart_form = true
						else
							POS_DATA.members[selected_user_to_do_smth[Name(player)]] = true
							local dta = core.serialize(POS_DATA.members)
							META:set_string("members", dta)
						end
					else
						str_err = X..S("Invalid player")
						immediate_restart_form = true
					end
				else
					str_err = X.." !!!"
					immediate_restart_form = true
				end
			end
			if immediate_restart_form then
				core.show_formspec(Name(player), "engine:areas_config", formspecinfo(Name(player), POS_DATA.members, selected_user_to_do_smth[Name(player)], ((str_err ~= "" and str_err) or ""), LVL[Name(player)], I))
			end
			if POS_DATA then cache_areas_poss[player:get_player_name()] = POS_DATA; end
		end
	end
end)

--object

core.register_entity("engine:entities__display__security", {
	physical = false,
	collisionbox = {0, 0, 0, 0, 0, 0},
	visual = "wielditem",
	visual_size = {x = 1.0 / 1.5, y = 1.0 / 1.5},
	textures = {"engine:areas_display_1"},
	timer = 0,
	on_step = function(self, dtime)
		self.timer = self.timer + dtime
		-- remove after 5 seconds
		if self.timer > 5 then
			self.object:remove()
		end
	end,
})

core.register_entity("engine:entities__display__security_2", {
	physical = false,
	collisionbox = {0, 0, 0, 0, 0, 0},
	visual = "wielditem",
	visual_size = {x = 1.0 / 1.5, y = 1.0 / 1.5},
	textures = {"engine:areas_display_2"},
	timer = 0,
	on_step = function(self, dtime)
		self.timer = self.timer + dtime
		-- remove after 5 seconds
		if self.timer > 5 then
			self.object:remove()
		end
	end,
})

for _, idk in pairs({{LVL=2, RAD=MineStars.Areas.Radius*2}, {LVL=1, RAD=MineStars.Areas.Radius}}) do
	local x = idk.RAD
	core.register_node("engine:areas_display_"..idk.LVL, {
		tiles = {"engine_protect_display.png"},
		use_texture_alpha = true,
		walkable = false,
		drawtype = "nodebox",
		node_box = {
			type = "fixed",
			fixed = {
				-- sides
				{-(x+.55), -(x+.55), -(x+.55), -(x+.45), (x+.55), (x+.55)},
				{-(x+.55), -(x+.55), (x+.45), (x+.55), (x+.55), (x+.55)},
				{(x+.45), -(x+.55), -(x+.55), (x+.55), (x+.55), (x+.55)},
				{-(x+.55), -(x+.55), -(x+.55), (x+.55), (x+.55), -(x+.45)},
				-- top
				{-(x+.55), (x+.45), -(x+.55), (x+.55), (x+.55), (x+.55)},
				-- bottom
				{-(x+.55), -(x+.55), -(x+.55), (x+.55), -(x+.45), (x+.55)},
				-- middle (surround protector)
				{-.55,-.55,-.55, .55,.55,.55},
			},
		},
		selection_box = {
			type = "regular",
		},
		paramtype = "light",
		groups = {dig_immediate = 3, not_in_creative_inventory = 1},
		drop = "",
	})
end
