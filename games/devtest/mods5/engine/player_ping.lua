MineStars.Ping = {
	Huds = {},
	--Queue = {}, --using core.after
}

core.register_on_joinplayer(function(player)
	MineStars.Ping.Huds[Name(player)] = player:hud_add({
		hud_elem_type = "text",
		position = {x = 0.5, y = 0.5},
		offset = {x = 0, y = 45},
		alignment = {x = "center", y = "down"},
		text = "",
		number = 7850184,
	})
end)

function MineStars.Ping.Ping(player1, player2)
	player2:hud_change(MineStars.Ping.Huds[Name(player2)], "text", MineStars.Translator("@1 Did animate you! Look around", Name(player1)))
	core.after(3, function(name) if IsOnline(name) then Player(name):hud_change(MineStars.Ping.Huds[name], "text", ""); end; end, Name(player2))
end

function MineStars.Ping.Process(player1, player2)
	--IF SNEAKING THEN AUTODISABLE ALL, only ping
	local controls = player1:get_player_control()
	--Check Player Config
	local meta1 = player1:get_meta()
	if meta1 then
		local int = meta1:get_int("TypeOfPing") --0=Ping, 1=PlayerInfo
		if not controls.sneak then
			if int > 0 then
				MineStars.Player.ShowPlayerInfoTo(player1, player2)
			else
				MineStars.Ping.Ping(player1, player2)
			end
		else
			MineStars.Ping.Ping(player1, player2)
		end
	end
end

core.register_on_rightclickplayer(function(player, clicker)
	MineStars.Ping.Process(clicker, player)
end)

for _, NAME in pairs({"toggle_ping", "cambiar_ping"}) do
	core.register_chatcommand(NAME, {
		params = "",
		func = BuildCmdFuncFor(function(name)
			if Player(name) then
				local meta = Player(name):get_meta();
				local int = meta:get_int("TypeOfPing");
				local str = MineStars.Translator"Done!";
				if int > 0 then
					int = 0;
					str = MineStars.Translator"On right click to players you will ping to they";
				else
					int = 1;
					str = MineStars.Translator"On right click to players you will see a menu with their info";
				end
				meta:set_int("TypeOfPing", int)
				return true, core.colorize("lightgreen", C_..str)
			else
				return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Must be connected!"))
			end
		end),
	})
end

function MineStars.Ping.DoesPingSomeone(player, msg)
	for _, p in pairs(core.get_connected_players()) do
		if msg:match(" "..p:get_player_name().." ") then
			return true
		end
	end
	return false
end




















