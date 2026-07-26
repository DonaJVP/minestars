--[[core.register_chatcommand("lobby", {
	params = "<params>",
	description = "/spawn",
	func = BuildCmdFuncFor(function(name, arg)
		if Player(name) then
			if not arg then
				local spawn_point = core.setting_get_pos("static_spawnpoint")
				if spawn_point then
					Player(name):set_pos(CheckPos(spawn_point))
				end
			else
				if core.check_player_privs(name, {server=true}) then
					local pos = CheckPos(vector.apply(Player(name):get_pos(), math.round))
					core.settings:set("static_spawnpoint", core.pos_to_string(pos))
				end
			end
		end
	end)
})--]]


-- MineStars Spawn Command "Writable"
-- Made by D. J. V. P. (Logiki-NiSep9)
-- https://nato.dx.com.ar/index.php
--License: LGPLv2.1+

spw = {pos_not_set = false}
spw.pos = {
	lobby = vector.new(),
	city = vector.new(),
}

local S = core.get_translator("engine")

if core.setting_get_pos("static_spawnpoint") then
	spw.pos.lobby = minetest.setting_get_pos("static_spawnpoint")
else
	core.log("error", "Spawn pos not set! Fallback to (0,0,0)")
	spw.pos_not_set = true
end

if core.setting_get_pos("city_pos") then
	spw.pos.city = minetest.setting_get_pos("city_pos")
else
	core.log("error", "City pos not set! Fallback to (0,0,0)")
end

core.register_chatcommand("spawn", {
	description = "Spawn",
	params = "[set]",
	func = function(name, params)
		local player = core.get_player_by_name(name)
		if player then
			if params:match("set") then
				if core.check_player_privs(name, {server=true}) then
					spw.pos.lobby = vector.floor(player:get_pos())
					core.settings:set("static_spawnpoint", core.pos_to_string(spw.pos.lobby))
					core.settings:write()
					spw.pos_not_set = false --fixbug
					return true, core.colorize("lightgreen", "-!- "..S("Done!"))
				else
					return true, core.colorize("#FF7C7C", "-!- "..S("No enough permissions to modify the spawn position!"))
				end
			else
				if not spw.pos_not_set then
					player:set_pos(spw.pos.lobby)
					return true, core.colorize("lightgreen", "-!- "..S("Teleporting..."))
				else
					return true, core.colorize("#FF7C7C", "-!- "..S("Position for spawn is not set!"))
				end
			end
		end
	end
})

core.register_chatcommand("city", {
	description = "City",
	params = "[set]",
	func = function(name, params)
		local player = core.get_player_by_name(name)
		if player then
			if params:match("set") then
				if core.check_player_privs(name, {server=true}) then
					spw.pos.city = vector.floor(player:get_pos())
					core.settings:set("city_pos", core.pos_to_string(spw.pos.city))
					core.settings:write()
					return true, core.colorize("lightgreen", "-!- "..S("Done!"))
				else
					return true, core.colorize("#FF7C7C", "-!- "..S("No enough permissions to modify the city position!"))
				end
			else
				player:set_pos(spw.pos.city)
				return true, core.colorize("lightgreen", "-!- "..S("Teleporting..."))
			end
		end
	end
})

core.register_on_joinplayer(function(player)
	local name = player:get_player_name()
	if name then
		if spw.pos_not_set then
			if core.check_player_privs(name, {server = true}) then
				core.chat_send_player(name, core.colorize("#FF7C7C", "-!-"..S("Please set a place for /spawn").."\n"..S("Use \"/spawn set\" to set a place")))
			end
		end
	end
end)