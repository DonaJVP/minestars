MineStars.Emotes = {
	Registered = {},
	PlayersDoing = {},
	Reverse = {},
}

function MineStars.Emotes.RegisterEmote(def)
	MineStars.Emotes.Registered[def.Tname or def.Name] = def
end

function MineStars.Emotes.SetEmote(player, name)
	name = MineStars.Emotes.Reverse[name]
	local animation_lengh = MineStars.Emotes.Registered[name].AnimationXY
	local IsLoop = MineStars.Emotes.Registered[name].Loop
	Player(player):set_animation(animation_lengh, MineStars.Emotes.Registered[name].Speed or 15, 0, false or IsLoop or false)
	MineStars.Emotes.PlayersDoing[Name(player)] = name
	
	--
	
	MineStars.HeadsObjects[player:get_player_name()]:set_bone_position("Head", {x = 0, y = 6.65 + MineStars.Emotes.Registered[name].SumY, z = MineStars.Emotes.Registered[name].SumZ}, {x = -math.deg(player:get_look_vertical()), y = -184, z = 1})
	
	player:set_player_sum(MineStars.Emotes.Registered[name].SumY, MineStars.Emotes.Registered[name].SumZ)
	return true
end

function MineStars.Emotes.StopEmote(player)
	--if MineStars.Emotes.PlayersDoing[Name(player)] then
		Player(player):set_animation({x=0, y=79}, 30, 0, true)
		MineStars.Emotes.PlayersDoing[Name(player)] = nil
		return true
	--end
end

core.register_on_leaveplayer(function(player)
	MineStars.Emotes.PlayersDoing[Name(player)] = nil
end)

function MineStars.Emotes.IsEmote(txt)
	for NAME in pairs(MineStars.Emotes.Registered) do
		if txt == NAME:lower() then MineStars.Emotes.Reverse[NAME:lower()] = NAME; return true end
	end
end

function MineStars.Emotes.ReturnStringTable()
	local _ = {}
	for NAME, DTA in pairs(MineStars.Emotes.Registered) do
		table.insert(_, NAME)
	end
	return _
end

core.register_chatcommand("emote", {
	params = "<emote | list(ar) | stop/parar>",
	description = MineStars.Translator("Emote"),
	func = BuildCmdFuncFor(function(name, emote)
		if Player(name) then
			if emote then
				if emote == "list" or emote == "listar" then
					return true, core.colorize("lightgreen", "➤ Emotes: "..table.concat(MineStars.Emotes.ReturnStringTable(), ", "))
				elseif emote == "stop" or emote == "parar" then
					MineStars.Emotes.StopEmote(Player(name))
					return
				else
					if MineStars.Emotes.IsEmote(emote) then
						MineStars.Emotes.SetEmote(Player(name), emote)
					end
				end
			else
				return true, core.colorize("#FF7C7C", "✗"..MineStars.Translate("Unknown emote or command"))
			end
		else
			return true, core.colorize("#FF7C7C", "✗"..MineStars.Translate("Must be connected!"))
		end
	end)
})

--
-- Emotes
--

MineStars.Emotes.RegisterEmote({
	Name = "Zombie",
	AnimationXY = {x = 1110, y = 1130},
	Loop = true,
	Speed = 15,
	SumY = 0,
	SumZ = 0,
})

MineStars.Emotes.RegisterEmote({
	Name = MineStars.Translator("Wave"),
	Tname = "Wave",
	AnimationXY = {x = 430, y = 470},
	Speed = 17,
	SumY = 0,
	SumZ = 0,
})

MineStars.Emotes.RegisterEmote({
	Name = MineStars.Translator("Applause"),
	Tname = "Applause",
	AnimationXY = {x = 480, y = 530},
	Speed = 30,
	SumY = 0,
	SumZ = 0,
})

MineStars.Emotes.RegisterEmote({
	Name = "Sit",
	AnimationXY = {x = 81, y = 160},
	Loop = true,
	Speed = 30,
	SumY = -5.5,
	SumZ = 0,
})

MineStars.Emotes.RegisterEmote({
	Name = "Lay",
	AnimationXY = {x = 162, y = 166},
	Loop = true,
	Speed = 30,
	SumY = -12.4,
	SumZ = -4,
})














