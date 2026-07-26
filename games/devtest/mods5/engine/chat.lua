--Chat
--[[
	==> 1^
	Catars: Hola
	[VIP] Catars: Hola
	[VIP-Wayfarer] Catars: Hola
	[Wayfarer] Catars: Hola
	# Depends what the user wants
	2 types
	==> 2^
	Catars: Hola
	VIP Catars:
	Lvl:13 & Vip Catars:
	Lvl:13 Catars:
	
--]]

MineStars.Chat = {}

function MineStars.Chat.GetTypeOfChatMsg(__obj)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local int = meta:get_int("type_of_chat_send")
			if int == 0 or not int then
				meta:set_int("type_of_chat_send", 1)
				int = 1
			end
			return int
		end
	end
end

core.register_on_chat_message(function(name, message)
	print(name..">"..message);
	if name and message then
		print("got")
		if Player(name) then --Security
			if MineStars.Vanish.Active[name] then
				core.chat_send_player(name, "Vanish is active!")
				return true
			end
			if MineStars.Ping.DoesPingSomeone(nil, message) then
				message = core.colorize("green", message)
			end
			local privs = core.get_player_privs(name)
			--if MineStars.StrictWordChecking.CheckMSG(message) then
				if privs.shout then
					local type_of_chat = MineStars.Chat.GetTypeOfChatMsg(name)
					if type_of_chat == 1 then
						local label = (MineStars.Labels.GetLabel(name) ~= "" and MineStars.Labels.GetLabel(name)) or nil
						local label_c = "#FFFFFF"
						if label then
							label_c = MineStars.Labels.GetLabelColor(label)
						end
						local rank = MineStars.XP.GetLevel(MineStars.XP.GetXP(name))
						local bool, fct = MineStars.Factions.GetFactionForPlayer(name)
						local fct_name = (bool and fct.Name) or ""
						local fct_conc = (fct_name ~= "" and (fct_name and "["..fct_name.."]")) or ""
						local fct_color = (fct ~= nil and fct.Color) or "lightgrey"
						local to_concatenate = (fct_name ~= "" and core.colorize(fct_color, (fct_conc ~= "" and fct_conc and fct_conc.." "))) or ""
						if (label and label ~= "") and rank.NotNewbie then
							to_concatenate = to_concatenate.."["..core.colorize(label_c, label).."-"..core.colorize(rank.ColorSTR, rank.RankName).."] "..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
							--core.chat_send_all("1")
						elseif label and label ~= "" then
							to_concatenate = to_concatenate.."["..core.colorize(label_c, label).."] "..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
							--core.chat_send_all("2")
						elseif rank.NotNewbie then
							to_concatenate = to_concatenate.."["..core.colorize(rank.ColorSTR, rank.RankName).."] "..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
							--core.chat_send_all("3")
						else
							to_concatenate = to_concatenate..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
						end
						MineStars.Chat.SendAll("1", to_concatenate, name)
						if MineStars.HTTP then
							discord.send(to_concatenate)
						end
						--MineStars.TextBubble.CreateNConcat(name, message)
						return true
					elseif type_of_chat == 2 then
						local label = (MineStars.Labels.GetLabel(name) ~= "" and MineStars.Labels.GetLabel(name)) or nil
						local label_c = "#FFFFFF"
						if label then
							label_c = MineStars.Labels.GetLabelColor(label)
						end
						local bool, fct = MineStars.Factions.GetFactionForPlayer(name)
						local fct_name = (bool and fct.Name) or ""
						local fct_conc = (fct_name ~= "" and (fct_name and "["..fct_name.."]")) or ""
						local fct_color = (fct ~= nil and fct.Color) or "lightgrey"
						local rank = MineStars.XP.GetLevel(MineStars.XP.GetXP(name))
						local to_concatenate = (fct_name ~= "" and core.colorize(fct_color, (fct_conc ~= "" and fct_conc and fct_conc.." "))) or ""
						if label and rank.NotNewbie then --
							to_concatenate = to_concatenate..core.colorize(rank.ColorSTR, "Lvl:"..rank.RankLvl).." & "..core.colorize(label_c, label).." "..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
						elseif label then
							to_concatenate = to_concatenate..core.colorize(label_c, label).." "..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
						elseif rank.NotNewbie then
							to_concatenate = to_concatenate..core.colorize(rank.ColorSTR, "Lvl:"..rank.RankLvl).." "..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
						else
							to_concatenate = to_concatenate..core.colorize(MineStars.Player.GetColor(name), name)..": "..message
						end
						--core.chat_send_all(to_concatenate)
						MineStars.Chat.SendAll("1", to_concatenate, name)
						if MineStars.HTTP then
							discord.send(to_concatenate)
						end
					else
						core.log("error", "In Chat: Could not determine what type of chat will use "..name.." this is a bug")
					end
					--MineStars.TextBubble.CreateNConcat(name, message)
					return true
				end
			--end
		end
	end
end)
local S = MineStars.Translator
core.register_chatcommand("chatmode", {
	description = MineStars.Translator("Type of chat in game"),
	params = "<1-2>",
	func = function(n, p)
		local params = p:split(" ")[1]
		if params then
			local player = Player(n)
			if player then
				local meta = player:get_meta()
				if tonumber(params) and meta then
					meta:set_int("type_of_chat_send", tonumber(params))
					core.chat_send_player(n, core.colorize("lightgreen", "✔"..S("Done!")))
				else
					core.chat_send_player(n, core.colorize("#FF7C7C", "✗"..S("Not a number!")))
				end
			else
				core.chat_send_player(n, core.colorize("#FF7C7C", "✗"..S("Not connected!")))
			end
		else
			core.chat_send_player(n, "✔"..S("Type Of chats:"))
			core.chat_send_player(n, "➢"..S("1: @1: Lorem Ipsum", n))
			core.chat_send_player(n, "➢"..S("1: [VIP] @1: Lorem Ipsum", n))
			core.chat_send_player(n, "➢"..S("1: [VIP-Wayfarer] @1: Lorem Ipsum", n))
			core.chat_send_player(n, "➢"..S("2: @1: Lorem Ipsum", n))
			core.chat_send_player(n, "➢"..S("2: VIP @1: Lorem Ipsum", n))
			core.chat_send_player(n, "➢"..S("2: Lvl:13 & VIP @1: Lorem Ipsum", n))
		end
	end
})

--
-- API
--

function MineStars.Chat.CheckIfMuted(player, pname)
	local meta = player:get_meta()
	if meta then
		local data = core.deserialize(meta:get_string("muted_people")) or {}
		if data then
			if data[pname] then
				return true
			end
		end
	end
	return false
end

function MineStars.Chat.SendAll(typ_, msg, pname)
	local LimitChat = (pname ~= nil);
	local Arguments = typ_ ~= "1" and typ_:split(":") or {[1] = "", [2] = "normal"}; --if typ_ is 1 then send without any decoration
	local String = "";
	local Color = "#FFFFFF";
	if Arguments and Arguments[1] and Arguments[2] then
		--Type of string
		if Arguments[1] == "notif" then
			String = arrow;
		elseif Arguments[1] == "good" then
			String = check;
		elseif Arguments[1] == "bad" then
			String = X;
		else
			String = Arguments[1];
		end
		--Color
		if Arguments[2] == "good" then
			Color = "lightgreen";
		elseif Arguments[2] == "bad" then
			Color = "#FF7C7C";
		elseif Arguments[2] == "normal" then
			Color = "#CECECE";
		else
			Color = Arguments[2];
		end
		--Send
		if LimitChat then
			for _, p in pairs(core.get_connected_players()) do
				local config = MineStars.Chat.CheckIfMuted(p, pname)
				if not config then
					core.chat_send_player(Name(p), core.colorize(Color, String..msg))
				end
			end
		else
			for _, p in pairs(core.get_connected_players()) do
				core.chat_send_player(Name(p), core.colorize(Color, String..msg))
			end
		end
	end
end
--[[
for 
	core.register_chatcommand("")
--]]

for _, CMDNAME in pairs({"mute", "silenciar"}) do
	core.register_chatcommand(CMDNAME, {
		description = MineStars.Translator("Mute a player to you"),
		params = "<player>",
		func = BuildCmdFuncFor(function(name, arg1)
			local player = Player(name)
			if player then
				local meta = player:get_meta()
				if meta then
					local str = core.deserialize(meta:get_string("muted_people")) or {}
					if str then
						if arg1 then
							if str[arg1] then
								return true, core.colorize("#FF7C7C", X..MineStars.Translator("You already muted @1", arg1))
							else
								str[arg1] = true
								meta:set_string("muted_people", core.serialize(str))
								--return nothing, its already handled by cmd builder
							end
						else
							return true, core.colorize("#FF7C7C", X..MineStars.Translator"You must specify a player!")
						end
					end
				end
			else
				return true, core.colorize("#FF7C7C", X..MineStars.Translator("Must be connected!"))
			end
		end)
	})
end

for _, CMDNAME in pairs({"unmute", "desilenciar"}) do -- Por si no saben, segun el RAE la palabra "desilenciar" es una palabra.
	core.register_chatcommand(CMDNAME, {
		description = MineStars.Translator"Unmute a player",
		params = "<player>",
		func = BuildCmdFuncFor(function(name, arg1)
			local player = Player(name)
			if player then
				local meta = player:get_meta()
				if meta then
					local data = core.deserialize(meta:get_string("muted_people")) or {}
					if data then
						if arg1 then
							if not data[arg1] then
								return true, core.colorize("#FF7C7C", X..MineStars.Translator("@1 is not muted", arg1))
							else
								data[arg1] = nil
								meta:set_string("muted_people", core.serialize(data))
								return
							end
						else
							return true, core.colorize("#FF7C7C", X..MineStars.Translator"You must specify a player!")
						end
					else
						return true, core.colorize("#FF7C7C", X..MineStars.Translator"You has not muted players!")
					end
				end
			else
				return true, core.colorize("#FF7C7C", X..MineStars.Translator("Must be connected!"))
			end
		end)
		
	})
end





















