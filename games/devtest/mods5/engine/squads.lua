--Here we might use that classic storage
MineStars.Factions = {}
local storage = MineStars.Storage
local S = MineStars.Translator

--Initialize the storage

--fastaccess

local PlayersInTeam = {}
local Teams = {} -- fastaccess
local TeamsID = {}
local BasesPos = {}
local Territories = {}

X="✗"
C_="✔"
check="✔"
arrow="➤"

do
	local playersinteams = core.deserialize(storage:get_string("PlayersInTeam")) or {}
	PlayersInTeam = playersinteams -- Fast access
	if not core.deserialize(storage:get_string("PlayersInTeam")) then
		storage:set_string("PlayersInTeam", "return {}")
	end
	local tid = core.deserialize(storage:get_string("TeamsID")) or {}
	TeamsID = tid -- Fast access
	if not core.deserialize(storage:get_string("TeamsID")) then
		storage:set_string("TeamsID", "return {}")
	end
	local bsp = core.deserialize(storage:get_string("BasesPos")) or {}
	BasesPos = bsp -- Fast access
	if not core.deserialize(storage:get_string("BasesPos")) then
		storage:set_string("TeamsID", "return {}")
	end
	local trs = core.deserialize(storage:get_string("TerritoryTeams")) or {}
	Territories = trs
	if not core.deserialize(storage:get_string("TerritoryTeams")) then
		storage:set_string("TerritoryTeams", "return {}")
	end
end

local function UpdateNSaveDatabase()
	local data = core.serialize(PlayersInTeam)
	storage:set_string("PlayersInTeam", data)
	
	local data = core.serialize(TeamsID)
	storage:set_string("TeamsID", data)
	
	local data_ = core.serialize(BasesPos)
	storage:set_string("BasesPos", data_)
	
	local tsrt = core.serialize(Territories)
	storage:set_string("TerritoryTeams", tsrt)
end

function MineStars.Factions.InitializeFaction(name_for, include)
	local ID = FormRandomString(4)
	local data = {
		Players = {[include] = true},
		Admins = {[include] = true}, --First person
		CanPlayersInviteOtherPlayers = false,
		BasePos = vector.round(Player(include):get_pos()),
		Name = name_for,
		Color = "#FFFFFF",
		ID = ID,
	}
	
	local serialized = core.serialize(data)
	storage:set_string("Team_"..ID, serialized)
	Teams[ID] = data
	TeamsID[name_for] = ID
	PlayersInTeam[include] = ID
	BasesPos[data.BasePos] = ID
	Territories[ID] = {
		[1] = {[core.pos_to_string(data.BasePos)] = 2}, -- fallback table
		[2] = data.BasePos, --points
	}
	UpdateNSaveDatabase()
	return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Your faction is already!"))
end

function MineStars.Factions.RemoveFaction(ID)
	if not Teams[ID] then
		local data = core.deserialize(storage:get_string("Team_"..ID))
		if data then
			TeamsID[data.Name] = nil
			for name, id in pairs(PlayersInTeam) do
				if id == ID then
					PlayersInTeam[name] = nil
				end
			end
			for pstr, pos in pairs(data.ChestsPos or {}) do
				if pos then
					local meta = core.get_meta(pos)
					if meta then
						meta:from_table({
							formspec = nil,
							infotext = nil,
							inventory = {},
						})
					end
					core.set_node(pos, {name = "air"})
				end
			end
			Teams[ID] = nil
		else
			return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction with ID: @1", ID))
		end
		UpdateNSaveDatabase()
		return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
	else
		local data = table.copy(Teams[ID])
		if data then
			TeamsID[data.Name] = nil
			for name, id in pairs(PlayersInTeam) do
				if id == ID then
					PlayersInTeam[name] = nil
				end
			end
			for pstr, pos in pairs(data.ChestsPos or {}) do
				if pos then
					local meta = core.get_meta(pos)
					if meta then
						meta:from_table({
							formspec = nil,
							infotext = nil,
							inventory = {},
						})
					end
					core.set_node(pos, {name = "air"})
				end
			end
			Teams[ID] = nil
		else
			return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction with ID: @1", ID))
		end
		UpdateNSaveDatabase()
		return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
	end
	return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Unexpected use of the function RemoveFaction in arguments"))
end

function MineStars.Factions.GetFaction(ID)
	if not Teams[ID] then
		local odnaiauabe = core.deserialize(storage:get_string("Team_"..ID))
		if odnaiauabe then
			Teams[ID] = odnaiauabe
			return odnaiauabe
		end
	else
		return Teams[ID]
	end
end

function MineStars.Factions.AddPlayerTo(ID, pname)
	if not Teams[ID] then
		local odnaiauabe = core.deserialize(storage:get_string("Team_"..ID))
		if odnaiauabe then
			Teams[ID] = odnaiauabe
			Teams[ID].Players[pname] = true
			PlayersInTeam[pname] = ID
			UpdateNSaveDatabase()
			local serialized = core.serialize(Teams[ID])
			storage:set_string("Team_"..ID, serialized)
			return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction existing which name has ID: @1", ID))
		end
	else
		Teams[ID].Players[pname] = true
		PlayersInTeam[pname] = ID
		UpdateNSaveDatabase()
		local serialized = core.serialize(Teams[ID])
		storage:set_string("Team_"..ID, serialized)
		return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
	end
end

function MineStars.Factions.RemovePlayerFrom(ID, pname)
	if not Teams[ID] then
		local odnaiauabe = core.deserialize(storage:get_string("Team_"..ID))
		if odnaiauabe then
			Teams[ID] = odnaiauabe
			if Teams[ID].Players[pname] then
				Teams[ID].Players[pname] = nil
				Teams[ID].Admins[pname] = nil
				PlayersInTeam[pname] = nil
				UpdateNSaveDatabase()
				local serialized = core.serialize(Teams[ID])
				storage:set_string("Team_"..ID, serialized)
				return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
			else
				return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Theres no player named @1", pname))
			end
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction existing which name has ID: @1", ID))
		end
	else
		if Teams[ID].Players[pname] then
			Teams[ID].Players[pname] = nil
			PlayersInTeam[pname] = nil
			Teams[ID].Admins[pname] = nil
			UpdateNSaveDatabase()
			local serialized = core.serialize(Teams[ID])
			storage:set_string("Team_"..ID, serialized)
			return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Theres no player named @1", pname))
		end
	end
end

function MineStars.Factions.DoAdminTo(ID, pname)
	if not Teams[ID] then
		local odnaiauabe = core.deserialize(storage:get_string("Team_"..ID))
		if odnaiauabe then
			Teams[ID] = odnaiauabe
			if Teams[ID].Players[pname] then
				Teams[ID].Admins[pname] = true
				local serialized = core.serialize(Teams[ID])
				storage:set_string("Team_"..ID, serialized)
				return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
			else
				return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Theres no player named @1", pname))
			end
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction existing which name has: ID:@1", ID))
		end
	else
		if Teams[ID].Players[pname] then
			Teams[ID].Admins[pname] = true
			local serialized = core.serialize(Teams[ID])
			storage:set_string("Team_"..ID, serialized)
			return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Theres no player named @1", pname))
		end
	end
end

function MineStars.Factions.RevokeAdminTo(ID, pname)
	if not Teams[ID] then
		local odnaiauabe = core.deserialize(storage:get_string("Team_"..ID))
		if odnaiauabe then
			Teams[ID] = odnaiauabe
			if Teams[ID].Admins[pname] then
				Teams[ID].Admins[pname] = nil
				local serialized = core.serialize(Teams[ID])
				storage:set_string("Team_"..ID, serialized)
				return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
			else
				return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Theres no admin named @1 in @2", pname, Teams[ID].Name))
			end
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction existing which name has ID: @1", ID))
		end
	else
		if Teams[ID].Admins[pname] then
			Teams[ID].Admins[pname] = nil
			local serialized = core.serialize(Teams[ID])
			storage:set_string("Team_"..ID, serialized)
			return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Theres no admin named @1 in @2", pname, Teams[ID].Name))
		end
	end
end

function MineStars.Factions.ApplySpecialKeywordWithValueIn(ID, keyword, value)
	if not Teams[ID] then
		local odnaiauabe = core.deserialize(storage:get_string("Team_"..ID))
		if odnaiauabe then
			Teams[ID] = odnaiauabe
			Teams[ID][keyword] = value
			local serialized = core.serialize(Teams[ID])
			storage:set_string("Team_"..ID, serialized)
			return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction existing which name has ID: @1", ID))
		end
	else
		Teams[ID][keyword] = value
		local serialized = core.serialize(Teams[ID])
		storage:set_string("Team_"..ID, serialized)
		return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
	end
end

function MineStars.Factions.SendToChatToFaction(ID, msg)
	msg = msg or "__"
	if not Teams[ID] then
		local data = core.deserialize(storage:get_string("Team_"..ID))
		if data then
			Teams[ID] = data
			for name in pairs(data.Players) do
				if IsOnline(name) then
					core.chat_send_player(name, core.colorize(data.Color or "lightblue", msg))
				end
			end
			return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
		else
			return false, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No faction existing which name has ID: @1", ID))
		end
	else
		for name in pairs(Teams[ID].Players) do
			if IsOnline(name) then
				core.chat_send_player(name, core.colorize(Teams[ID].Color or "lightblue", msg))
			end
		end
		return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
	end
end

--Load any thing
local S = MineStars.Translator
local T = function(_) __ = {} for ___ in pairs(_) do table.insert(__, ___) end return __ end

---->
--Invites Engine>
---->

--Invites[pname] = {ByPlayer="", ToID=9f7a5f}
local Invites = {}
function MineStars.Factions.InvitePlayerTo(pname, ID, byplayer)
	if Player(pname) and Player(byplayer) then
		if pname ~= byplayer then
			local fct = MineStars.Factions.GetFaction(ID)
			if fct then
				if (fct.Admins[byplayer] or fct.CanPlayersInviteOtherPlayers) then
					if not fct.Players[pname] then
						Invites[pname] = {ByPlayer = byplayer, ToID = ID}
						core.chat_send_player(pname, core.colorize("lightblue", "➢"..S("You got invited by @1 to their faction '@2'", byplayer, fct.Name)))
						return true, core.colorize("lightgreen", "✔"..S("Invite to @1 have been sent!", pname)), true
					else
						return true, core.colorize("#FF7C7C", "✗"..S("@1 is already in the faction!", pname)), false
					end
				else
					return true, core.colorize("#FF7C7C", "✗"..S("Insufficient permissions!")), false
				end
			else
				return true, core.colorize("#FF7C7C", "✗"..S("Faction don't exist!")), false
			end
		else
			return true, core.colorize("#FF7C7C", "✗"..S("Can not invite ur self!")), false
		end
	else
		return true, core.colorize("#FF7C7C", "✗"..S("Not connected!")), false
	end
end
function MineStars.Factions.AcceptInvite(pname)
	if Player(pname) and Invites[pname] then
		local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
		if not bool then
			local invitor = Invites[pname].ByPlayer
			local ID = Invites[pname].ToID
			local fct = MineStars.Factions.GetFaction(ID)
			if fct and (fct.Admins[invitor] or fct.CanPlayersInviteOtherPlayers) then
				MineStars.Factions.AddPlayerTo(ID, pname)
				if Player(invitor) then
					core.chat_send_player(invitor, core.colorize("lightgreen", "✔"..S("@1 Has joined your faction!", pname)))
				end
				Invites[pname] = nil
				return true, core.colorize("lightgreen", "✔"..S("You joined @1 faction!", fct.Name))
			else
				return true, core.colorize("#FF7C7C", "✗"..S("Server error, sorry!").."\nUnknown admin "..invitor)
			end
		else
			return true, core.colorize("#FF7C7C", "✗"..S("You are in a faction!"))
		end
	else
		return true, core.colorize("#FF7C7C", "✗"..S("You aren't invited to any faction!"))
	end
	return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Unexpected use of the function AcceptInvite in arguments"))
end
function MineStars.Factions.DeclineInvite(pname)
	if Invites[pname] then
		local invitor = Invites[pname].ByPlayer
		if Player(invitor) then
			core.chat_send_player(invitor, core.colorize("#FF7C7C", "✗"..S("@1 Cancelled your invitation", pname)))
		end
		Invites[pname] = nil
		return true, core.colorize("lightgreen", "✔"..S("Done!"))
	else
		return true, core.colorize("#FF7C7C", "✗"..S("You aren't invited to any faction!"))
	end
	return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Unexpected use of the function DeclineInvite in arguments"))
end

---->
--Getters>
---->

function MineStars.Factions.GetFactionForPlayer(__obj)
	local name = Name(__obj)
	if name then
		local ID = PlayersInTeam[name]
		if ID then
			if Teams[ID] then
				return true, Teams[ID]
			else
				--That might be unloaded, load it!
				local data = core.deserialize(storage:get_string("Team_"..ID))
				if data then
					Teams[ID] = data --Save into cache
					return true, data
				else
					return false, nil, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Theres corrupted database, please report it! ID: @1 Name: @2",ID,name))
				end
			end
		else
			return false, nil, core.colorize("#FF7C7C", "✗"..MineStars.Translator("No Faction for @1", name))
		end
	end
	return false, nil, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Unexpected use of the function GetFactionForPlayer in arguments"))
end


---->
--Commands>
---->

local function returnformspec(pname)
	local bool, fct = MineStars.Factions.GetFactionForPlayer(pname)
	
	local formspec = "formspec_version[9]size[11.5,4.2]box[0,0;11.5,0.5;#99CCFF]hypertext[0.05,0;6,3;a;<style color=#FFFFFF size=20>"..S("Help menu")..((fct ~= nil and " | ") or "")..((fct and fct.Name) or "").."</style>]container[0.2,1]"
	
	--See what he can do:
	
	--MANAGEMENT
	if fct then
		if fct.Admins[pname] then
			formspec = formspec.."label[0,-0.20;✔ "..S("menu: Management menu").."]"
		else
			formspec = formspec.."label[0,-0.20;✗ "..S("menu: Management menu").."]"
		end
	else
		formspec = formspec.."label[0,-0.20;✗ "..S("menu: Management menu").."]"
	end
	
	--INFO
	if fct then
		formspec = formspec.."label[0,0.10;✔ "..S("info: Information of your faction").."]"
	else
		formspec = formspec.."label[0,0.10;✗ "..S("info: Information of your faction").."]"
	end
	
	--MAKE
	if not fct then
		formspec = formspec.."label[0,0.40;✔ "..S("make: Make a new faction, only applicable when you are not on any other faction").."]"
	else
		formspec = formspec.."label[0,0.40;✗ "..S("make: Make a new faction, only applicable when you are not on any other faction").."]"
	end
	
	--LEAVE
	if fct then
		formspec = formspec.."label[0,0.70;✔ "..S("leave: Abandon your faction").."]"
	else
		formspec = formspec.."label[0,0.70;✗ "..S("leave: Abandon your faction").."]"
	end
	
	--JOIN
	if Invites[pname] then
		formspec = formspec.."label[0,1.00;✔ "..S("join: Join a faction, @1 has invited you to their faction", Invites[pname].ByPlayer).."]"
	else
		formspec = formspec.."label[0,1.00;✗ "..S("join: Join a faction, only applicable when you are invited").."]"
	end
	
	--DECLINE
	if Invites[pname] then
		formspec = formspec.."label[0,1.30;✔ "..S("decline: Decline a faction invite (You've got an invite from @1)", Invites[pname].ByPlayer).."]"
	else
		formspec = formspec.."label[0,1.30;✗ "..S("decline: Decline a faction invite").."]"
	end
	
	--TP
	if fct then
		formspec = formspec.."label[0,1.60;✔ "..S("tp: Teleport to your faction base").."]"
	else
		formspec = formspec.."label[0,1.60;✗ "..S("tp: Teleport to your faction base").."]"
	end
	
	--INVITE
	if fct then
		if fct.Admins[pname] or fct.CanPlayersInviteOtherPlayers then
			formspec = formspec.."label[0,1.90;✔ "..S("invite: Invite a player to your faction").."]"
		else
			formspec = formspec.."label[0,1.90;✗ "..S("invite: Invite a player to your faction").."]"
		end
	else
		formspec = formspec.."label[0,1.90;✗ "..S("invite: Invite a player to your faction").."]"
	end
	
	--KICK
	if fct then
		if fct.Admins[pname] then
			formspec = formspec.."label[0,2.20;✔ "..S("kick: Kick a player of your faction").."]"
		else
			formspec = formspec.."label[0,2.20;✗ "..S("kick: Kick a player of your faction").."]"
		end
	else
		formspec = formspec.."label[0,2.20;✗ "..S("kick: Kick a player of your faction").."]"
	end
	
	return formspec.."container_end[]style[quit;bgcolor=red;textcolor=white]button_exit[0.1,3.5;11.3,0.6;quit;Exit]"
end

local cache_sqd_names = {}

local function returnformspeceditor(def, stcerr, name, boolean, admins_I, players_I)
	local form = "formspec_version[9]"..
	"size[10,6.2]"..
	"box[0,0;10,0.5;#99CCFF]"..
	"hypertext[0.05,0;6,3;a;<style color=#FFFFFF size=20>Management menu | "..def.Name.."</style>]"..
	"container[0.2,1]"..
	"field[-0.1,-0.1;3,0.5;field_name_string;Name of your faction;"..def.Name.."]"..
	"field[-0.1,0.75;3,0.5;field_color_string;Color of your faction;"..def.Color.."]"..
	"field[3,-0.1;3,0.5;playername;Player Name;"..((cache_sqd_names[name] ~= nil and cache_sqd_names[name]) or "").."]"..
	"style[btn_setpos;bgcolor=#12F4FF;textcolor=white]"..
	"style[btn_invite;bgcolor=green;textcolor=white]"..
	"style[btn_kick;bgcolor=red;textcolor=white]"..
	"style[btn_doadmin;bgcolor=red;textcolor=yellow]"..
	"style[btn_revokeadmin;bgcolor=red;textcolor=yellow]"..
	"style[btn_gotopos;bgcolor=blue;textcolor=white]"..
	"style[btn_pvp;bgcolor=orange;textcolor=white]"..
	"style[quitsave;bgcolor=green;textcolor=white]"..
	"button[-0.1,1.35;3,0.7;btn_setpos;Set Base Pos here]"..
	"button[-0.1,2.15;3,0.7;btn_gotopos;Go to Base Pos]"..
	"button[3,0.55;1.45,0.7;btn_invite;Invite]"..
	"button[4.6,0.55;1.45,0.7;btn_kick;Kick]"..
	"button[3,1.35;3.03,0.7;btn_doadmin;Do admin]"..
	"button[3,2.15;3.03,0.7;btn_revokeadmin;Revoke admin]"..
	"button[-0.1,2.9;6.13,0.7;btn_pvp;Enable PvP (F vs F) {"..(((boolean == nil) and "false") or ((boolean == false) and "false") or ((boolean == true) and "true")):upper().."}]"..
	"textlist[6.2,-0.1;3.5,2;players;"..table.concat(T(def.Players), ",")..";"..(players_I or 1).."]"..
	"textlist[6.2,2.3;3.5,2;admins;"..table.concat(T(def.Admins), ",")..";"..(admins_I or 1).."]"..
	"label[6.2,-0.25;Players]"..
	"label[6.2,2.15;Admins]"..
	"label[0,3.87;Coordinates:]"..
	"label[1.8,3.87;"..core.pos_to_string(def.BasePos).."]"..
	"label[0,4.25;"..(stcerr or S("No errors reported!")).."]"..
	"container_end[]"..
	"style[quit;bgcolor=red;textcolor=white]"..
	"button_exit[0.1,5.5;4.6,0.6;quit_;Quit]"..
	"button[4.81,5.5;5.1,0.6;quitsave;Guardar-Save]"..
	"field_enter_after_edit[field_name_string;true]"..
	"field_enter_after_edit[field_color_string;true]"
	return form
end

local function rp(p)
	return p.x..","..p.y..","..p.z
end

local function returnchestformspec(pos, name)
	return "formspec_version[1]" ..
	"size[8,9]" ..
	"box[-0.3,-0.3;8.38,0.35;#00E1FF]" ..
	"label[0,-0.38;"..name.."]" ..
	"list[nodemeta:"..rp(pos)..";main;0,0.3;8,4;]" ..
	"list[current_player;main;0,4.85;8,1;]" ..
	"list[current_player;main;0,6.08;8,3;8]" ..
	"listring[nodemeta:"..rp(pos)..";main]" ..
	"listring[current_player;main]"
end

local double_click_rmv = {}

core.register_chatcommand("faction", {
	params = "<help>",
	description = MineStars.Translator("Manage your faction, or see that faction..."),
	func = BuildCmdFuncFor(function(name, cmd1, arg1, arg2)
		if Player(name) then
			local n = name
			if cmd1 then
				if cmd1 == "help" then
					core.chat_send_player(n, "✔"..S("Help menu"))
					core.chat_send_player(n, "➢"..S("menu: Management menu"))
					core.chat_send_player(n, "➢"..S("info: Information of your faction"))
					core.chat_send_player(n, "➢"..S("make: Make a new faction, only applicable when you are not on any other faction"))
					core.chat_send_player(n, "➢"..S("leave: Abandon your faction"))
					core.chat_send_player(n, "➢"..S("join: Join a faction, only applicable when you are invited"))
					core.chat_send_player(n, "➢"..S("decline: Decline a faction invite"))
					core.chat_send_player(n, "➢"..S("tp: Teleport to your faction base"))
					core.chat_send_player(n, "➢"..S("remove: Remove your faction"))
					core.chat_send_player(n, "➢"..S("kick: Kick a player of your faction"))
					core.chat_send_player(n, "➢"..S("invite: Invite a player to your faction"))
					core.show_formspec(n, "squadsinfo", returnformspec(n))
				elseif cmd1 == "info" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						return true, inf
					else
						if data then
							core.chat_send_player(n, "✔"..S("Faction @1", data.Name))
							core.chat_send_player(n, "➢"..S("Players: @1", table.concat(T(data.Players), ", ")))
							core.chat_send_player(n, "➢"..S("Admins: @1", table.concat(T(data.Admins), ", ")))
							core.chat_send_player(n, "➢"..S("ID: @1", data.ID))
							core.chat_send_player(n, "➢"..S("Base pos: @1", core.pos_to_string(data.BasePos)))
						end
					end
				elseif cmd1 == "make" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						if arg1 then
							local bool, str = MineStars.Factions.InitializeFaction(arg1, name)
							return bool, str
						else
							return true, core.colorize("#FF7C7C", "✗"..S("Invalid use of the command!").."\n/faction make <name-nombre>")
						end
					else
						return true, core.colorize("#FF7C7C", "✗"..S("You already are in a faction, leave it if you want to make other faction"))
					end
				elseif cmd1 == "leave" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						return true, core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!"))
					else
						local bool, inf = MineStars.Factions.RemovePlayerFrom(data.ID, name)
						return bool, inf or core.colorize("lightgreen", "✔"..S("Done!"))
					end
				elseif cmd1 == "join" then
					local bool, inf = MineStars.Factions.AcceptInvite(name)
					return bool, inf
				elseif cmd1 == "decline" then
					local b, i = MineStars.Factions.DeclineInvite(name)
					return b, i
				elseif cmd1 == "tp" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						return true, core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!"))
					else
						local pos = CheckPos(data.BasePos)
						local player = Player(name)
						if player then
							player:set_pos(pos)
							return nil, nil --handled by chat command function builder
						else
							return true, core.colorize("#FF7C7C", "✗"..S("Not connected!"))
						end
					end
				elseif cmd1 == "kick" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						return true, core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!"))
					else
						if arg1 then
							if data.Admins[name] then
								local b,i=MineStars.Factions.RemovePlayerFrom(data.ID, arg1)
								return b,i
							else
								return true, core.colorize("#FF7C7C", "✗"..S("You aren't a admin!"))
							end
						else
							return true, core.colorize("#FF7C7C", "✗"..S("Invalid use of the command!").."\n/faction kick <name-nombre>")
						end
					end
				elseif cmd1 == "invite" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						return true, core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!"))
					else
						if arg1 then
							local b, i = MineStars.Factions.InvitePlayerTo(arg1, data.ID, name)
							return b, i
						else
							return true, core.colorize("#FF7C7C", "✗"..S("Invalid use of the command!").."\n/faction invite <name-nombre>")
						end
					end
				elseif cmd1 == "menu" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						return true, core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!"))
					else
						if data.Admins[name] then
							core.show_formspec(name, "squadseditor", returnformspeceditor(data, nil, name, data.PvP))
						else
							return true, core.colorize("#FF7C7C", "✗"..S("You aren't a admin!"))
						end
					end
				elseif cmd1 == "remove" then
					local bool, data, inf = MineStars.Factions.GetFactionForPlayer(name)
					if not bool then
						return true, core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!"))
					else
						if not double_click_rmv[name] then
							double_click_rmv[name] = 1
							return true, core.colorize("#FF7C7C", "➢"..S("Are you sure to delete your faction?"))
						elseif double_click_rmv[name] == 1 then
							core.chat_send_player(name, core.colorize("#FF7C7C", "➢"..S("Deleting... Please wait!")))
							local b, i = MineStars.Factions.RemoveFaction(data.ID)
							return b, i
						end
					end
				else
					return true, core.colorize("#FF7C7C", "✗"..S("Unrecognized command, usage:").."\n/faction <command> <argument>")
				end
			else
				return true, core.colorize("#FF7C7C", "✗"..S("Invalid use of the command, usage:").."\n/faction <command> <argument>")
			end
		else
			return true, core.colorize("#FF7C7C", "✗"..S("Must be connected!"))
		end
	end)
})

local function switcher(bool)
	if not bool then return true end
	if boot then return false end
end

local cache_open_chests = {}

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "squadseditor" then
		local _, def = MineStars.Factions.GetFactionForPlayer(player)
		if fields.quitsave then
			--Error for failures
			local field = ""
			local thereserror = false
			--Field
			if fields.field_name_string ~= nil and fields.field_name_string ~= def.Name then
				if fields.field_name_string == "" or fields.field_name_string == " " then
					--core.show_formspec(player:get_player_name(), "squadseditor", returnformspeceditor(def, "✗"..S("Invalid name for faction!")))
					field = core.colorize("#FF7C7C", "✗"..S("Invalid name for faction!"))
					thereserror = true
				else
					if fields.field_name_string:len() <= 20 then
						MineStars.Factions.ApplySpecialKeywordWithValueIn(def.ID, "Name", fields.field_name_string)
					else
						field = core.colorize("#FF7C7C", "✗"..S("Name is too big (Limit is 20chars)"))
						thereserror = true
					end
				end
			end
			if fields.field_color_string ~= nil and fields.field_color_string ~= def.Color then
				MineStars.Factions.ApplySpecialKeywordWithValueIn(def.ID, "Color", fields.field_color_string)
			end
			if field ~= "" then
				core.show_formspec(player:get_player_name(), "squadseditor", returnformspeceditor(def, field, Name(player), def.PvP))
				return
			end
			return
		end
		
		if fields.playername and fields.playername ~= "" and fields.playername ~= " " then
			cache_sqd_names[player:get_player_name()] = fields.playername
		end
		
		local immediate_restart_form = false
		
		--Buttons btn_={setpos,gotopos,invite,kick,doadmin,revokeadmin,pvp}
		do
			if fields.btn_setpos then
				local pos_ = CheckPos(vector.round(player:get_pos()))
				MineStars.Factions.ApplySpecialKeywordWithValueIn(def.ID, "BasePos", pos_)
				BasesPos[core.pos_to_string(pos_)] = def.ID
				UpdateNSaveDatabase()
				immediate_restart_form = true
			end
			if fields.btn_gotopos then
				local pos = CheckPos(def.BasePos)
				player:set_pos(vector.round(pos))
			end
			if fields.btn_invite then
				local name = fields.playername or cache_sqd_names[player:get_player_name()]
				if name then
					if name ~= Name(player) then
						local b, i = MineStars.Factions.InvitePlayerTo(name, def.ID, Name(player))
						if b then
							if not thereserror then
								field = i
								immediate_restart_form = true
							end
						end
					else
						field = core.colorize("#FF7C7C", "✗"..S("Can not invite your self!"))
						immediate_restart_form = true
					end
				end
			end
			if fields.btn_kick then
				local name = fields.playername or cache_sqd_names[player:get_player_name()]
				if name then
					if name ~= Name(player) then
						local b, i = MineStars.Factions.RemovePlayerFrom(def.ID, name)--MineStars.Factions.InvitePlayerTo(name, def.ID, Name(player))
						if b then
							if not thereserror then
								field = i
								immediate_restart_form = true
							end
						end
					else
						field = core.colorize("#FF7C7C", "✗"..S("Can not kick your self!"))
						immediate_restart_form = true
					end
				end
			end
			if fields.btn_doadmin then
				local name = fields.playername or cache_sqd_names[player:get_player_name()]
				if name then
					if name ~= Name(player) then
						local b, i = MineStars.Factions.DoAdminTo(def.ID, name)--MineStars.Factions.InvitePlayerTo(name, def.ID, Name(player))
						if b then
							if not thereserror then
								field = i
								immediate_restart_form = true
							end
						end
					else
						field = core.colorize("#FF7C7C", "✗"..S("Can not make your self admin!"))
						immediate_restart_form = true
					end
				end
			end
			if fields.btn_revokeadmin then
				local name = fields.playername or cache_sqd_names[player:get_player_name()]
				if name then
					if name ~= Name(player) then
						local b, i = MineStars.Factions.RevokeAdminTo(def.ID, name)--MineStars.Factions.InvitePlayerTo(name, def.ID, Name(player))
						if b then
							if not thereserror then
								field = i
								immediate_restart_form = true
							end
						end
					else
						field = core.colorize("#FF7C7C", "✗"..S("Can not revoke your self admin!"))
						immediate_restart_form = true
					end
				end
			end
			if fields.btn_pvp then
				local newbool = switcher(def.PvP)
				MineStars.Factions.ApplySpecialKeywordWithValueIn(def.ID, "PvP", newbool)
				immediate_restart_form = true
			end
		end
		--TEXTLIST
		local I = 1
		if fields.admins then
			local exp = core.explode_textlist_event(fields.admins)
			if exp.type == "CHG" then
				local IDX__ = T(def.Admins)
				local I_STR = IDX__[exp.index]
				cache_sqd_names[player:get_player_name()] = I_STR
				immediate_restart_form = true
			end
			I = exp.index
		end
		local I2 = 1
		if fields.players then
			local exp = core.explode_textlist_event(fields.players)
			if exp.type == "CHG" then
				local IDX__ = T(def.Players)
				local I_STR = IDX__[exp.index]
				cache_sqd_names[player:get_player_name()] = I_STR
				immediate_restart_form = true
			end
			I2 = exp.index
		end
		if immediate_restart_form then
			core.show_formspec(player:get_player_name(), "squadseditor", returnformspeceditor(def, field, Name(player), def.PvP, I, I2))
		end
	elseif formname == "engine:faction_chest_form" then
		if fields.quit then
			local posdata = cache_open_chests[Name(player)]
			if posdata then
				core.swap_node(posdata.pos, {name = "engine:faction_chest", param2 = posdata.param2})
			end
		end
	else
		if cache_open_chests[Name(player)] then
			local posdata = cache_open_chests[Name(player)]
			if posdata then
				core.swap_node(posdata.pos, {name = "engine:faction_chest", param2 = posdata.param2})
			end
		end
	end
end)

core.register_node("engine:faction_chest", {
	description = S("Faction chest"),
	legacy_facedir_simple = true,
	groups = {choppy = 2, oddly_breakable_by_hand = 2},
	tiles = {
		"default_chest_top.png",
		"default_chest_top.png",
		"default_chest_side.png^faction_flag.png",
		"default_chest_side.png^faction_flag.png",
		"default_chest_side.png^faction_flag.png",
		"default_chest_front.png^faction_flag.png",
	},
	paramtype2 = "facedir",
	is_ground_content = false,
	on_construct = function(pos)
		local meta = core.get_meta(pos)
		local inv = meta:get_inventory()
		inv:set_size("main", 8*4)
	end,
	after_place_node = function(pos, placer)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(placer)
		if fct then
			local meta = core.get_meta(pos)
			meta:set_string("infotext", S("Faction @1", fct.Name))
			meta:set_string("fct_id", fct.ID)
			local t = fct.ChestsPos or {}
			t[core.pos_to_string(pos)] = pos
			MineStars.Factions.ApplySpecialKeywordWithValueIn(fct.ID, "ChestsPos", t)
		end
	end,
	on_rightclick = function(pos, node, clicker, itemstack, pointed_thing)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(clicker)
		local bruteforce = core.check_player_privs(clicker:get_player_name(), {protection_bypass=true})
		if fct or bruteforce then
			local meta = core.get_meta(pos)
			if meta then
				local ID = meta:get_string("fct_id")
				if (ID and ID ~= "") or bruteforce then
					if fct then
						if (ID == fct.ID) or bruteforce then
							meta:set_string("infotext", S("Faction @1", fct.Name))
							core.show_formspec(clicker:get_player_name(), "engine:faction_chest_form", returnchestformspec(pos, fct.Name))
							core.swap_node(pos, {name = "engine:faction_chest_open", param2 = node.param2})
							cache_open_chests[Name(clicker)] = {pos = pos, param2 = node.param2}
						end
					else
						core.show_formspec(clicker:get_player_name(), "engine:faction_chest_form", returnchestformspec(pos, "BUG! OnRightClick: Unknown Faction"))
					end
				end
			end
		end
	end,
	allow_metadata_inventory_put = function(pos, listname, index, stack, player)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(player)
		if fct or core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
			local meta = core.get_meta(pos)
			if meta:get_string("fct_id") == fct.ID then
				meta:set_string("infotext", S("Faction @1", fct.Name))
				return stack:get_count()
			end
		end
		return 0
	end,
	allow_metadata_inventory_take = function(pos, listname, index, stack, player)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(player)
		if fct or core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
			local meta = core.get_meta(pos)
			if meta:get_string("fct_id") == fct.ID then
				meta:set_string("infotext", S("Faction @1", fct.Name))
				return stack:get_count()
			end
		end
		return 0
	end,
	allow_metadata_inventory_move = function(pos, from_list, from_index, to_list, to_index, count, player)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(player)
		if fct or core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
			local meta = core.get_meta(pos)
			if meta:get_string("fct_id") == fct.ID then
				meta:set_string("infotext", S("Faction @1", fct.Name))
				return count
			end
		end
		return 0
	end,
	can_dig = function(pos, player)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(player)
		local meta = core.get_meta(pos)
		local inv = meta:get_inventory()
		--print("9")
		if (fct and fct.ID == meta:get_string("fct_id")) or core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
			if inv:is_empty("main") then
				--print("d")
				if fct then
					local t = fct.ChestsPos or {}
					local str = core.pos_to_string(pos)
					t[str] = nil
					MineStars.Factions.ApplySpecialKeywordWithValueIn(fct.ID, "ChestsPos", t)
				end
				return true
			else
				--print("j")
				if core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
					if fct then
						local t = fct.ChestsPos or {}
						local str = core.pos_to_string(pos)
						t[str] = nil
						MineStars.Factions.ApplySpecialKeywordWithValueIn(fct.ID, "ChestsPos", t)
					end
					return true
				else
					return false
				end
			end
		end
		--print("c")
		return false
	end,--move the below script to up
	--[[on_dig = function(pos, node, player)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(player)
		local meta = core.get_meta(pos)
		local ID = meta:get_string("fct_id")
		print("8")
		if fct or core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
			if (ID == fct.ID) or core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
				print("p")
				local str = core.pos_to_string(pos)
				local t = fct.ChestsPos or {}
				t[str] = nil
				MineStars.Factions.ApplySpecialKeywordWithValueIn(fct.ID, "ChestsPos", t)
				return true
			else
				print("a")
				return false
			end
		else
			print("b")
			if not core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
				return false
			else
				return true
			end
		end
	end,--]]
	on_place = function(itemstack, placer, pointed_thing)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(placer)
		if fct then
			return core.item_place(itemstack, placer, pointed_thing)
		else
			core.chat_send_player(Name(placer), core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!")))
			return itemstack
		end
	end,
	on_blast = function() end,
})
--[[
MineStars.Areas.RegisterOnPlaceNodeUnderCondition({
	NodeName = "engine:faction_chest",
	Condition = function(player)
		local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(player)
		
	end,
})
--]]
core.register_node("engine:faction_chest_open", {
	description = "Engine faction chest, state=open",
	can_dig = function() return false end,
	on_blast = function() end,
	drawtype = "mesh",
	mesh = "chest_open.obj",
	visual = "mesh",
	paramtype = "light",
	paramtype2 = "facedir",
	selection_box = {
		type = "fixed",
		fixed = { -1/2, -1/2, -1/2, 1/2, 3/16, 1/2 },
	},
	groups = {not_in_creative_inventory = 1},
	tiles = {
		"default_chest_top.png",
		"default_chest_top.png",
		"default_chest_side.png^faction_flag.png",
		"default_chest_side.png^faction_flag.png",
		"default_chest_front.png^faction_flag.png",
		"default_chest_inside.png"
	},
})

core.register_lbm({
	label = "Close opened chests on load, engine->factionchest",
	name = "engine:faction_chest_open___",
	nodenames = {"engine:faction_chest_open"},
	run_at_every_load = true,
	action = function(pos, node)
		node.name = "engine:faction_chest"
		core.swap_node(pos, node)
	end
})

MineStars.PvP.RegisterCallback(function(player, hitter, damage, canpvp_player, canpvp_hitter)
	if player:is_player() and hitter:is_player() then
		--Data load
		local b1,fct1,inf1 = MineStars.Factions.GetFactionForPlayer(player)
		local b2,fct2,inf2 = MineStars.Factions.GetFactionForPlayer(hitter)
		
		if not fct1 then return end
		if not fct2 then return end
		
		--go ahead line
		if canpvp_player and canpvp_hitter then
			--Calc by ID and get val
			if fct1.ID ~= fct2.ID then
				if not (fct1.PvP and fct2.PvP) then
					return true
				end
			else
				return true
			end
		end
	end
end)

MineStars.Areas.RegisterCheckFunc(function(player, pos)
	local bool, fct, inf = MineStars.Factions.GetFactionForPlayer(player)
	--do
		--[[local is_near_someone = {ID="", got=false, pos=vector.new()}
		for pstr, ID in pairs(BasesPos) do
			local p = core.string_to_pos(pstr)
			if vector.distance(p, pos) <= 20 then
				is_near_someone = {ID=ID, got=true, pos=p}
				break
			end
		end
		if is_near_someone.got and not fct then
			local f_fct = MineStars.Factions.GetFaction(is_near_someone.ID)
			local f_nme = (f_fct and f_fct.Name) or "Unknown Faction!"
			return true, core.colorize("#FF7C7C", "✗"..S("You cant build here! Territory of "..f_nme))
		end--]]
		--territory [if not return of upper checker]
		local intersection = {bool=false, ID="", pos=vector.new()}
		for ID, territories in pairs(Territories) do
			for _, pos__ in pairs(territories) do
				if not type(pos__) == "table" then -- [psrt] = index
					if vector.distance(pos__, pos) <= 21 then
						intersection = {
							bool=true,
							ID=ID,
							pos=pos__,
						}
						break
					end
				end
			end
		end
		if intersection.bool then
			local f_fct = MineStars.Factions.GetFaction(intersection.ID)
			local f_nme = (f_fct and f_fct.Name) or "Unknown Faction!"
			return true, core.colorize("#FF7C7C", "✗"..S("You cant build here! Territory of @1", f_nme))
		end
	--end
	if fct then
		if intersection.got then
			if intersection.ID ~= fct.ID then
				--Get name
				local f_fct = MineStars.Factions.GetFaction(intersection.ID)
				local f_nme = (f_fct and f_fct.Name) or "Unknown Faction!"
				--Report him
				local b = MineStars.Factions.SendToChatToFaction(is_near_someone.ID, S("@1 Almost modified your faction territory, he/she are from faction @2", Name(player), fct.Name))
				local __ = b == false and (core.log("error", "Could not send message to "..is_near_someone.ID.." faction") or true) --Really complex?
				--Return true
				return true, core.colorize("#FF7C7C", "✗"..S("You cant build here! Territory of @1", f_nme))
			end
		end
	end
end)

local function queue_can_not_place_here_and_remove_block(pos, player)
	core.after(0.3, function(pos)
		core.set_node(pos, {name="air"})
	end)
	core.chat_send_player(Name(player), core.colorize("#FF7C7C", "✗"..S("You aren't in a faction!")))
end

local feq = function(p1,p2) return (p1.x == p2.x) and (p1.y == p2.y) and (p1.z == p2.z) end
local box = {type = "fixed", fixed = {{-0.2, -0.5, -0.2, 0.2, 2.1, 0.2},{-1.2,  1.2, -0.1, 0.2, 2.1, 0.1}}}
core.register_node("engine:factions_territory_pole", {
	description = S("Faction Territory Pole").."\n"..S("Place this item somewhere to extend your faction territory!"),
	node_box = box,
	selection_box = box,
	drawtype = "mesh",
	mesh = "flag_pole.obj",
	tiles = {"blue_flag__.png"},
	visual_scale = 0.40000,
	pointable = true,
	sunlight_propagates = true,
	walkable = false,
	diggable = true,
	groups = {cracky = 2},
	paramtype = "light",
	buildable_to = false,
	after_place_node = function(pos, placer)
		local b, f, i = MineStars.Factions.GetFactionForPlayer(placer)
		if f then
			local meta = core.get_meta(pos)
			meta:set_string("infotext", "Faction \""..f.Name.."\"")
			meta:set_string("id_of_faction", f.ID)
			table.insert(Territories[f.ID], pos)
		else
			queue_can_not_place_here_and_remove_block(pos, placer)
		end
	end,
	can_dig = function(pos, player)
		local meta=core.get_meta(pos)
		local b, f, i = MineStars.Factions.GetFactionForPlayer(placer)
		if f then
			if f.ID == meta:get_string("id_of_faction") then
				local i
				for _, pos_ in pairs(Territories[f.ID]) do
					if feq(pos_, pos) then
						i = _
						break
					end
				end
				if i then
					table.remove(Territories[f.ID], i)
				end
				return true
			else
				if core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
					return true
				end
			end
		else
			if core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
				return true
			end
		end
	end,
})





