core.registered_chatcommands["admin"].func = function(n)
	core.chat_send_player(n, "The admins of the server are: Logiki"..((", " and (((core.settings:get("admins") ~= "") or "") and core.settings:get("admins"))) or ""))
end
local l_f = core.registered_chatcommands["privs"].func
core.registered_chatcommands["privs"].func = function(n, p)
	if core.get_player_privs(n) and core.get_player_privs(n)["server"] then
		return l_f(n,p)
	else
		return true, ">>> No enough privs to check!"
	end
end

local function form_status()
	return {
		Name = "MineStars",
		EngineName = core.get_version().project,
		EngineVersion = core.get_version().string,
		Server = {
			PerStepMS = MineStars.Lag,
			MaxLagFromSteps = "UNKNOWN",--core.get_server_max_lag()
			MultiThreadedServer = true,
		},
		ActivePlayers = #core.get_connected_players()
	}
end

--[[
function core.get_server_status(name, joined)
	return "# Server|MineStars|Heavenly = {Lag = '"..MineStars.Lag.."', MaxLag = '"..core.get_server_max_lag().."', Players = "..#core.get_connected_players().."} Engine: {Name='"..core.get_version().project.."', Version="..core.get_version().string.."'}"
	return dump2(form_status())
end--]]
