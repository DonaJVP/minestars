MineStars.Rules = {
	Langs = {
		["EN"] = {
			"1. No Swearing & No Cursing",
			"2. Don't use hacks",
			"3. No Spam",
			"4. No ask for privs",
			"5. Respect everyone",
			"6. No Griefing",
			"7. Do not mention other servers",
		},
		["ES"] = {
			"1. No decir groserias",
			"2. No usar hacks",
			"3. No Spam",
			"4. No preguntar para privs",
			"5. Respete a todos",
			"6. No mencione otros servidores",
			"7. No Griefing",
		},
	},
	return_formspec = function(lang)
		return "size[4,3.3]" ..
		"box[-0.3,-0.3;4.37,0.3;#55AA11]" ..
		"label[-0.2,-.39;Rules [ES/EN]]" ..
		"label[3.5,-.39;EN]" ..
		"textlist[-0.14,0.1;4.12,3;text;"..table.concat(MineStars.Rules.Langs[lang], ",")..";1;false]" ..
		"image_button[-0.14,3.2;1.5,0.6;blank.png;button_lang;Lang: "..lang.."]" ..
		"image_button_exit[1.2,3.2;3.015,0.6;blank.png;button_accept;Accept | Aceptar]"
	end,
	ActivePlayers = {},
}

core.register_on_newplayer(function(player)
	core.set_player_privs(Name(player), {shout = true})
	core.after(0.5, function(player)
		if Name(player) and IsOnline(Name(player)) then
			core.show_formspec(Name(player), "rules", MineStars.Rules.return_formspec("EN"))
			MineStars.Rules.ActivePlayers[Name(player)] = "EN"
		end
	end, player)
end)

local function inverse(I)
	return (I == "EN" and "ES") or "EN"
end

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "rules" then
		if fields.button_lang then
			local L = inverse(MineStars.Rules.ActivePlayers[Name(player)])
			core.show_formspec(Name(player), "rules", MineStars.Rules.return_formspec(L))
			MineStars.Rules.ActivePlayers[Name(player)] = L
		end
		if fields.button_accept then
			if not core.get_player_privs(Name(player)).interact then
				core.set_player_privs(Name(player), {shout = true, interact = true, tp = true, home = true})
			end
		end
	end
end)

core.register_chatcommand("rules", {
	params = "",
	description = "rules",
	func = function(name)
		if Player(name) then
			core.show_formspec(name, "rules", MineStars.Rules.return_formspec("EN"))
			MineStars.Rules.ActivePlayers[name] = "EN"
		else
			return true, core.colorize("#FF7C7C", X..MineStars.Translator("Must be connected!"))
		end
	end,
})
















































