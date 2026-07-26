-- MINESTARS
--[[
	Donatto Josue Viveros Pintos
	Creator of the core (heart) of the servers
	Enjoy <3
	CC-BY-SA Lic.
	Everything was made from zero, enjoy the optimized enviroment, not the shit of 20+ mods
	This mod has much more features!
--]]

MineStars = {}

assert(MineStars, "Secondary Engine not found.")

MineStars.ModPath = core.get_modpath("engine")
MineStars.Storage = core.get_mod_storage()
MineStars.Translator = core.get_translator("engine")
MineStars.HTTP = core.request_http_api()
MineStars.Lag = 0.1
MineStars.ExplainBugs = false

print2 = print;
--[[
if not MineStars.ExplainBugs then
	print = function(...)
		core.log("DUMP: "..dump({...}))
	end
else
	print = function()
	end
end--]]

MineStars.Translate = MineStars.Translator

local ant_dofile = dofile
function dofile(file)
	ant_dofile(file)
	core.log("action", "[ENGINE] Loaded "..file)
end

dofile(MineStars.ModPath.."/command_func_builder.lua")
dofile(MineStars.ModPath.."/helpers.lua")
dofile(MineStars.ModPath.."/player.lua")
dofile(MineStars.ModPath.."/pvp.lua")
dofile(MineStars.ModPath.."/areas.lua")
dofile(MineStars.ModPath.."/default_huds.lua")
dofile(MineStars.ModPath.."/xp.lua")
dofile(MineStars.ModPath.."/chat.lua")
dofile(MineStars.ModPath.."/discord.lua")
dofile(MineStars.ModPath.."/stamina.lua")
dofile(MineStars.ModPath.."/privs.lua")
dofile(MineStars.ModPath.."/swearing_network.lua")
dofile(MineStars.ModPath.."/labels.lua")
dofile(MineStars.ModPath.."/chatcommand_overriders.lua")
dofile(MineStars.ModPath.."/bank.lua")
dofile(MineStars.ModPath.."/offhand.lua")
dofile(MineStars.ModPath.."/squads.lua")
dofile(MineStars.ModPath.."/shield.lua")
dofile(MineStars.ModPath.."/physics.lua")
dofile(MineStars.ModPath.."/wielded_light.lua")
dofile(MineStars.ModPath.."/table_of_work.lua")
dofile(MineStars.ModPath.."/minerals.lua")
dofile(MineStars.ModPath.."/jobs.lua")
dofile(MineStars.ModPath.."/emotes.lua")
dofile(MineStars.ModPath.."/player_ping.lua")
dofile(MineStars.ModPath.."/cmd.lua")
dofile(MineStars.ModPath.."/discord.lua")
dofile(MineStars.ModPath.."/report_system.lua")
dofile(MineStars.ModPath.."/vanish.lua")
dofile(MineStars.ModPath.."/rules.lua")
dofile(MineStars.ModPath.."/tools.lua")
dofile(MineStars.ModPath.."/waypoint.lua")
dofile(MineStars.ModPath.."/multihome.lua")
dofile(MineStars.ModPath.."/item.lua")
dofile(MineStars.ModPath.."/recipes.lua")
dofile(MineStars.ModPath.."/light.lua")
dofile(MineStars.ModPath.."/totem.lua")
dofile(MineStars.ModPath.."/text.lua")
dofile(MineStars.ModPath.."/inventory.lua")
dofile(MineStars.ModPath.."/portals/do.lua")
dofile(MineStars.ModPath.."/chatcommands.lua")
dofile(MineStars.ModPath.."/custom_huds.lua")
dofile(MineStars.ModPath.."/old_client_support.lua")
dofile(MineStars.ModPath.."/magiccompass.lua")
dofile(MineStars.ModPath.."/soundeffects.lua")
dofile(MineStars.ModPath.."/weather.lua")
dofile(MineStars.ModPath.."/player_advantages.lua")
dofile(MineStars.ModPath.."/steps.lua")
dofile(MineStars.ModPath.."/head_entity.lua")
--dofile(MineStars.ModPath.."/skybox_environment.lua")
dofile(MineStars.ModPath.."/item_bags.lua")

dofile = ant_dofile


