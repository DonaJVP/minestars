--XP
MineStars.XP = {
	Huds = {},
	RegisteredXP_Levels = {},
	LastRegisteredXPLevel = {},
}

---->
--Data>
---->

--Fast access
local fastaccess = {}

function MineStars.XP.GetXP(__obj)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local val = fastaccess[Name(__obj)] or meta:get_int("xp_level")
			if (val == 0) or not val then
				val = 0
			end
			fastaccess[Name(__obj)] = val
			return val
		end
	end
end
function MineStars.XP.RemoveXP(__obj, value)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local val = fastaccess[Name(__obj)] or meta:get_int("xp_level")
			if (val == 0) or not val then
				val = 0
			end
			val = val - value
			val = AlwaysReturnPositiveInteger(val)
			fastaccess[Name(__obj)] = val
			meta:set_int("xp_level", val)
			MineStars.XP.UpdateHud(player)
			return true
		end
		MineStars.XP.UpdateHud(player)
	end
end
function MineStars.XP.AddXP(__obj, value)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local val = fastaccess[Name(__obj)] or meta:get_int("xp_level")
			if (val == 0) or not val then
				val = 0
			end
			local val2 = val + value
			val2 = AlwaysReturnPositiveInteger(val2)
			fastaccess[Name(__obj)] = val2
			meta:set_int("xp_level", val2)
			MineStars.XP.UpdateHud(player)
			--Check
			--MineStars.XP.CheckIfXPIsNew(lvl_old, lvl_new)
			local oldlevel = MineStars.XP.GetLevel(val)
			local newlevel = MineStars.XP.GetLevel(val2)
			if MineStars.XP.CheckIfXPIsNew(oldlevel, newlevel) then
				MineStars.XP.SendGotNewLvl(player, newlevel)
				core.after(0.4, MineStars.Player.UpdateNametag, player)
			end
			return true
		end
		MineStars.XP.UpdateHud(player)
	end
end

---->
--Chat>
---->

--Depends on player config......
function MineStars.XP.SendGotNewLvl(player, lvl)
	local meta = player:get_meta();
	local bool = GetBoolFromInt(meta:get_int("send_notif_of_new_rank"), true);
	local data = core.deserialize(meta:get_string("unlocked_ranks")) or {};
	if data[lvl.TechName] then
		return false; --return if player already unlocked the rank
	end
	if bool then
		data[lvl.TechName] = true;
		meta:set_string("unlocked_ranks", core.serialize(data));
		return true, MineStars.Chat.SendAll("notif:good", MineStars.Translator("@1 has unlocked @2!", Name(player), core.colorize(lvl.ColorSTR, lvl.RankName)), Name(player));
	else
		return false;
	end
end

---->
--Special Functions>
---->

function MineStars.XP.CheckIfXPIsNew(lvl_old, lvl_new)
	return lvl_old.TechName ~= lvl_new.TechName
end

function MineStars.XP.RegisterXPLevel(name, def)
	MineStars.XP.RegisteredXP_Levels[name] = def
	MineStars.XP.LastRegisteredXPLevel = def
	MineStars.XP.LastRegisteredXPLevel.TechName = name
end

function MineStars.XP.GetLevel(XP_LEVEL)
	for ranklvl, def in pairs(MineStars.XP.RegisteredXP_Levels) do
		if XP_LEVEL >= def.RankInteger.Min and XP_LEVEL <= def.RankInteger.Max then
			def.TechName = ranklvl
			return def
		end
	end
	return MineStars.XP.LastRegisteredXPLevel
end

---->
-- Cache Data
---->

MineStars.XP.CacheXP__NAME = {}

---->
--Huds>
---->

local number_of_sectors = 42

core.register_on_joinplayer(function(player)
	player:hud_add({
		hud_elem_type = "statbar",
		position = {x=0.5,y=1},
		scale = {x=1,y=1},
		text = "xp_bar_bg.png",
		number = number_of_sectors,
		alignment = {x=-1,y=-1},
		offset = {x=-175,y=-110},
		direction = 0,
		size = {x = 17, y = 17},
	})
	MineStars.XP.Huds[Name(player)] = player:hud_add({
		hud_elem_type = "statbar",
		position = {x=0.5,y=1},
		text = "xp_bar.png",
		number = 2,
		alignment = {x=-1,y=-1},
		offset = {x=-175,y=-110},
		direction = 0,
		size = {x = 17, y = 17},
	})
	core.after(0.5, function(player)
		if player then
			local XP = MineStars.XP.GetXP(player)
			local LVL = MineStars.XP.GetLevel(XP)
			if LVL then
				MineStars.XP.CacheXP__NAME[Name(player)] = LVL
			end
		end
		--08/02/25
		MineStars.XP.UpdateHud(player);
	end, player)
end)

core.register_on_leaveplayer(function(player)
	MineStars.XP.Huds[Name(player)] = nil
end)

--Update

function MineStars.XP.UpdateHud(__obj)
	local player = Player(__obj)
	if player then
		if MineStars.XP.Huds[Name(player)] then
			local xp_val = MineStars.XP.GetXP(__obj)
			local progress = number_of_sectors
			local rankdef = MineStars.XP.GetLevel(xp_val)
			if rankdef then
				progress = (xp_val-rankdef.RankInteger.Min)/(rankdef.RankInteger.Max-rankdef.RankInteger.Min)*number_of_sectors
				player:hud_change(MineStars.XP.Huds[Name(player)], "number", ReturnNonUpperLimit(progress, number_of_sectors))
			end
		end
	end
end
--[[
local steps = 0
core.register_globalstep(function(dt)
	steps = steps + dt
	if steps >= 0.5 then
		steps = 0
		for _, p in pairs(core.get_connected_players()) do
			MineStars.XP.UpdateHud(p)
		end
	end
end)
--]]
---->
--Functional>
---->

core.register_on_newplayer(function(player)
	local meta = player:get_meta()
	if meta then
		meta:set_int("xp_level", 0)
	end
end)

---->
--Ranks>
---->

MineStars.XP.RegisterXPLevel("Newbie", {
	RankName = "Newbie",
	RankInteger = {Min=0, Max=100},
	ColorSTR = "#90FFD3",
	RankLvl = 1,
	NotNewbie = false,
})

MineStars.XP.RegisterXPLevel("Player", {
	RankName = "Player",
	RankInteger = {Min=100, Max=200},
	ColorSTR = "#38FFF1",
	RankLvl = 2,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Regular", {
	RankName = "Regular",
	RankInteger = {Min=200, Max=800},
	ColorSTR = "#3CC0B7",
	RankLvl = 3,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Aspirant", {
	RankName = "Aspirant",
	RankInteger = {Min=800, Max=1600},
	ColorSTR = "#1A83FF",
	RankLvl = 4,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Freshman", {
	RankName = "Freshman",
	RankInteger = {Min=1600, Max=3200},
	ColorSTR = "#1A21FF",
	RankLvl = 5,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Wayfarer", {
	RankName = "Wayfarer",
	RankInteger = {Min=3200, Max=6400},
	ColorSTR = "#5E1AFF",
	RankLvl = 6,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Citizen", {
	RankName = "Citizen",
	RankInteger = {Min=6400, Max=12800},
	ColorSTR = "#9F1AFF",
	RankLvl = 7,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Veteran", {
	RankName = "Veteran",
	RankInteger = {Min=12800, Max=25600},
	ColorSTR = "#FF1AFD",
	RankLvl = 8,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Master", {
	RankName = "Master",
	RankInteger = {Min=25600, Max=51200},
	ColorSTR = "#FF1A91",
	RankLvl = 9,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Legend", {
	RankName = "Legend",
	RankInteger = {Min=51200, Max=102400},
	ColorSTR = "#FF1A1B",
	RankLvl = 10,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Mythic", {
	RankName = "Mythic",
	RankInteger = {Min=102400, Max=204800},
	ColorSTR = "#FF7900",
	RankLvl = 11,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Supreme", {
	RankName = "Supreme",
	RankInteger = {Min=204800, Max=409600},
	ColorSTR = "#1CFF00",
	RankLvl = 12,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("Champion", {
	RankName = "Champion",
	RankInteger = {Min=409600, Max=819200},
	ColorSTR = "#FFCD00",
	RankLvl = 13,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("King", {
	RankName = "King",
	RankInteger = {Min=819200, Max=1638400},
	ColorSTR = "#28B21F",
	RankLvl = 14,
	NotNewbie = true,
})

MineStars.XP.RegisterXPLevel("RepresentatorOfHeavenly", {
	RankName = "RepresentatorOfHeavenly",
	RankInteger = {Min=1638400, Max=3276800},
	ColorSTR = "#73B06E",
	RankLvl = 15,
	NotNewbie = true,
})

---->
--Luanti Utils>
---->

core.register_on_dignode(function(_, __, player)
	if player and player:is_player() then
		MineStars.XP.AddXP(player, 1)
		if __ and __.name and core.registered_items[__.name] and core.registered_items[__.name].groups and core.registered_items[__.name].groups.ore then
			--Add entities
			local m = math.random(1, 4)
			for i = 1, m do
				MineStars.XP.spawn_xp_obj(_, m)
			end
		end
	end
end)
core.register_on_placenode(function(_, __, player)
	if player and player:is_player() then
		MineStars.XP.AddXP(player, 1)
	end
end)
core.register_on_dieplayer(function(player, reason)
	MineStars.XP.RemoveXP(player, 20)
end)
core.register_on_craft(function(itemstack, player)
	MineStars.XP.AddXP(player, itemstack:get_count())
end)

---->
--XP object>
---->

core.register_entity("engine:xp", {
	initial_properties = {
		visual = "sprite",
		visual_size = {x=0.2, y=0.2, z=0.2},
		collision_box = {-0.05,-0.05,-0.05,0.05,0.05,0.05},
		textures = {"exp_tile.png"},
		pointable = false,
		physical = false,
		is_visible = true,
		backface_culling = false,
		makes_footstep_sound = false,
		static_save = false,
		glow = 10,
	},
	timer = 0,
	assigned_xp = 0,
	assigned_player_to_go = nil,
	on_step = function(self, dt, mv)
		if self and self.object then
			--self.object:set_acceleration({x=0,y=-8.1,z=0})
			--local colliding = mv.collides
			--if (not colliding) and (not mv.object) the
				if not self.assigned_player_to_go then
					local objs = core.get_objects_inside_radius(self.object:get_pos(), 4)
					if objs then
						for _, o in pairs(objs) do
							if o:is_player() then
								local direction = vector.direction(self.object:get_pos(), o:get_pos())
								self.object:set_velocity(vector.multiply(direction, 2))
								self.assigned_player_to_go = o:get_player_name()
							end
						end
					end
				else
					local player = Player(self.assigned_player_to_go)
					if player then
						local direction = vector.direction(self.object:get_pos(), player:get_pos())
						self.object:set_velocity(vector.multiply(direction, 2))
					else
						self.assigned_player_to_go = nil
					end
				end
				
				local player = Player(self.assigned_player_to_go)
				if player then
					local dist = vector.distance(player:get_pos(), self.object:get_pos())
					if dist < 1.2 then
						MineStars.XP.AddXP(player, self.assigned_xp or 0)
						self.object:remove()
					end
				end
			--else
			--	if mv.object then
			--		local obj = mv.object
			--		if obj:is_player() then
			--			xp_redo.add_xp(obj:get_player_name(), self.assigned_xp)
			--			self.object:remove()
			--		end
			--	end
			--end
		end
	end,
	on_punch = function() return true end,
})

function MineStars.XP.spawn_xp_obj(pos, xp)
	if pos and xp then
		local obj = core.add_entity(pos, "engine:xp")
		if obj then
			local ent = obj:get_luaentity()
			if ent then
				ent.assigned_xp = xp
			else
				obj:remove()
			end
		end
	end
end

local function vector_random(pos, rad)
	return {
		x = math.random(pos.x, pos.x + rad),
		y = pos.y,
		z = math.random(pos.z, pos.z + rad),
	}
end

local function subtract(pos)
	return vector.subtract(pos, vector.new(0,0.15,0))
end

--if mobs then
	table.insert(mobs.OnDie, function(self)
		if self then
			if (not self.owner) or self.owner == "" then
				if self.type == "monster" then
					if self.object:get_pos() then
						local randint = math.random(1, 4)
						local XP = math.random(10, 100) / randint
						for i = 1, randint, 1 do
							MineStars.XP.spawn_xp_obj(subtract(vector_random(self.object:get_pos(), 0.5)), XP)
						end
						return
					else
						core.log("error", "Failure to handle OnDie(): unknown object")
					end
				else
					if self.object:get_pos() then
						local randint = math.random(1, 4)
						local XP = math.random(5, 50) / randint
						for i = 1, randint, 1 do
							MineStars.XP.spawn_xp_obj(subtract(vector_random(self.object:get_pos(), 0.5)), XP)
						end
					else
						core.log("error", "Failure to handle OnDie(): unknown object")
					end
				end
			end
		end
	end)
--end


core.register_chatcommand("xp", {
	description = MineStars.Translator("Get your XP level and value"),
	func = function(name)
		local player = Player(name)
		if player then
			local XP = MineStars.XP.GetXP(player)
			local level = MineStars.XP.GetLevel(XP)
			return true, core.colorize("lightgreen", "Rank: "..core.colorize(level.ColorSTR, level.RankName).."\n".."XP: "..XP)
		else
			return true, core.colorize("#FF7C7C", MineStars.Translator("Must be connected!"))
		end
	end
})














