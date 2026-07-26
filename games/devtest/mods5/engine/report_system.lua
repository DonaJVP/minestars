--Api
local http = MineStars.HTTP
if not http then return end
--Functional
function SendToDiscordReports(data)
	local json = minetest.write_json(data)
	http.fetch({
		url = "https://discord.com/api/webhooks/1315687039726911550/PRQuDEWdYAyPgziUeHgIgE5Nze1qeupIEIWV2sNJeFaFxBHZQWIGufMcO2Co_thERuXW",
		method = "POST",
		extra_headers = {"Content-Type: application/json"},
		data = json
	}, function()
		-- doin nothin
	end)
end
function SendToDiscordAnnouncements(data)
	local json = minetest.write_json(data)
	http.fetch({
		url = "https://discord.com/api/webhooks/1315685818337071125/_xXHKCUWFJ64uCcVgApY9WpFjJ0-0Sn4gi_5wrFB5b1xzjMR0aSG8SzzaLu2rcSIGF7b",
		method = "POST",
		extra_headers = {"Content-Type: application/json"},
		data = json
	}, function()
		-- doin nothin
	end)
end
--Commands
--Reports
for _, NAME in pairs({"report", "reportar"}) do
	core.register_chatcommand(NAME, {
		description = MineStars.Translator("Report someone that has broke the rules of the server"),
		params = "<player> <reason|razón>",
		func = function(name, params)
			local data = params:split(" ")
			local player = data[1]
			local argument = ""
			for i = 2, #data do
				argument = argument.." "..data[i]
			end
			if player and argument then
				if argument:len() < 30 then
					SendToDiscordReports({
						content = ">> "..name.." reported "..player.." from "..argument
					})
				end
			else
				return true, core.colorize("#FF7C7C", "✗"..S("Invalid use of the command!").."\n/report <player> <reason|razón>")
			end
		end,
	})
end
--Announcements
for _, NAME in pairs({"announce", "anunciar"}) do
	core.register_chatcommand(NAME, {
		description = MineStars.Translator("Announce a place or something to the people"),
		params = "<argument|argumento>",
		func = function(name, params)
			local argument = (params ~= "" and params ~= " " and params) or nil
			if Player(name) then
				if argument then
					if argument:len() < 20 then
						SendToDiscordAnnouncements({
							content = ">> "..name..": "..argument.." "..core.pos_to_string(vector.apply(Player(name):get_pos(), math.round))
						})
						core.chat_send_all(core.colorize("lightgreen", ">>> "..name..": "..argument.." "..core.pos_to_string(vector.apply(Player(name):get_pos(), math.round))))
					end
				else
					return true, core.colorize("#FF7C7C", "✗"..S("Invalid use of the command!").."\n/announce <argument|argumento>")
				end
			else
				return true, core.colorize("#FF7C7C", "✗"..S("Must be connected!"))
			end
		end,
	})
end












