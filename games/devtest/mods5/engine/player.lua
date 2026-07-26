--Player
--[[

	META = {
		size = VECTOR,
	}

--]]

MineStars.Player = {
	IsOnline = {}
}

local get_connected_players = core.get_connected_players
local abs = math.abs
local deg = math.deg
local basepos = vector.new(0, 6.35, 0)
local lastdir = {}

minetest.register_on_leaveplayer(function(player)
	lastdir[player:get_player_name()] = nil
end)


local function GetFormspecInfo(playername, dug, placed, killed, rank, died, money, skinimg, editor_mode, petadata)
	if editor_mode then
		return "formspec_version[6]"..
		"size[9,5.6]"..
		"box[0,0;9,0.8;#99CCFF]"..
		"hypertext[0,0;6,3;a;<style color=#FFFFFF size=20>Player Info</style>]"..
		"label[0.09,0.58;"..playername.."]"..
		"container[0.2,1]"..
		"textarea[0,0;4,2;infeditor;;"..petadata.."]" ..
		"label[0,2.20;Nodes dug: "..dug.."]"..
		"label[0,2.50;Nodes placed: "..placed.."]"..
		"label[0,2.80;Died: "..died.."]"..
		"label[0,3.10;Killed: "..killed.."]"..
		"label[0,3.40;Rank: ".."Lvl:"..rank.RankLvl.." & "..rank.RankName.."]"..
		"label[0,3.70;Money: $"..money.."]"..
		"image[6,1;2.5,4.5;"..skinimg.."]"..
		"container_end[]"..
		"style[quit;bgcolor=green;textcolor=white]"..
		"button_exit[0.1,4.9;8.8,0.6;quit;Quit & Save]"
	else
		return "formspec_version[6]"..
		"size[9,5.6]"..
		"box[0,0;9,0.8;#99CCFF]"..
		"hypertext[0,0;6,3;a;<style color=#FFFFFF size=20>Player Info</style>]"..
		"label[0.09,0.58;"..playername.."]"..
		"container[0.2,1]"..
		"label[0,0;"..petadata[1].."]"..
		"label[0,0.30;"..petadata[2].."]"..
		"label[0,0.60;"..petadata[3].."]"..
		"label[0,0.90;"..petadata[4].."]"..
		"label[0,1.20;"..petadata[5].."]"..
		"label[0,1.50;"..petadata[6].."]"..
		"label[0,1.80;"..petadata[7].."]"..
		"label[0,2.20;Nodes dug: "..dug.."]"..
		"label[0,2.50;Nodes placed: "..placed.."]"..
		"label[0,2.80;Died: "..died.."]"..
		"label[0,3.10;Killed: "..killed.."]"..
		"label[0,3.40;Rank: ".."Lvl:"..rank.RankLvl.." & "..rank.RankName.."]"..
		"label[0,3.70;Money: $"..money.."]"..
		"image[6,1;2.5,4.5;"..skinimg.."]"..
		"container_end[]"..
		"style[quit;bgcolor=red;textcolor=white]"..
		"button_exit[0.1,4.9;8.8,0.6;quit;Quit]"
	end
end

function MineStars.Player.ShowPlayerInfoTo(n, params)
	n = Name(n)
	params = Name(params)
	local player = Player(params)
	if player and params ~= n then
		--Proceed with info
		local meta = player:get_meta()
		if meta then
			local i1 = meta:get_int("nodes_dug")
			local i2 = meta:get_int("nodes_placed")
			local i3 = meta:get_int("killed")
			local i4 = meta:get_int("died")
			local i5 = MineStars.XP.GetLevel(MineStars.XP.GetXP(player))
			local i6 = MineStars.Bank.GetValue(player)
			--bio
			local i7 = meta:get_string("bio")
			local petadata = {
				[1] = MineStars.Translator("No bio yet."),
				[2] = "   *      *      *    *    *",
				[3] = " *     *    *     *  *",
				[4] = "     *      *          *     *",
				[5] = "  *      *     *      *",
				[6] = "      *      *     *      *",
				[7] = "*    *     *     *     *",
			}
			if i7 and i7 ~= "" then
				local datas = i7:split("\n")
				for i, data in pairs(datas) do
					petadata[i] = data
				end
			end
			--skinimg
			local skin = skins.get_player_skin(player)
			local txt_head = skin:get_head()
			local i8 = txt_head
			--send
			core.show_formspec(n, FormRandomString(3), GetFormspecInfo(params, i1, i2, i3, i5, i4, i6, i8, false, petadata))
		end
	else
		local player = Player(n)
		local meta = player:get_meta()
		if meta then
			local i1 = meta:get_int("nodes_dug")
			local i2 = meta:get_int("nodes_placed")
			local i3 = meta:get_int("killed")
			local i4 = meta:get_int("died")
			local i5 = MineStars.XP.GetLevel(MineStars.XP.GetXP(player))
			local i6 = MineStars.Bank.GetValue(player)
			local i7 = meta:get_string("bio")
			--skinimg
			local skin = skins.get_player_skin(player)
			local txt_head = skin:get_head()
			local i8 = txt_head
			--send
			core.show_formspec(n, "playerinfoeditor", GetFormspecInfo(params, i1, i2, i3, i5, i4, i6, i8, true, i7))
		end
	end
end

---->
--Helpers>
---->

function IsOnline(pname)
	return MineStars.Player.IsOnline[pname]
end

---->
--Central>
---->

local fastaccess = {}

function MineStars.Player.GetColor(__obj)
	local player = Player(__obj)
	if player then
		local n = Name(__obj)
		if fastaccess[n] then
			return fastaccess[n]
		end
		local meta = player:get_meta()
		if meta then
			local str_color = meta:get_string("player_color")
			if str_color ~= "" then
				fastaccess[Name(__obj)] = str_color
				return str_color
			else
				return "#FFFFFF"
			end
		end
	end
end

function MineStars.Player.GetLastPlayerDeathPos(__obj)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local pos_str = meta:get_string("death_pos_str")
			local pos = (pos_str ~= "" and core.string_to_pos(pos_str)) or CheckPos(player:get_pos())
			return pos
		end
	end
	return vector.new()
end

function MineStars.Player.SetColor(__obj, colorstr)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			meta:set_string("player_color", colorstr)
			if player:is_player() then --stupid
				player:set_properties({nametag = core.colorize(colorstr, "["..(MineStars.XP.GetLevel(MineStars.XP.GetXP(player)) and MineStars.XP.GetLevel(MineStars.XP.GetXP(player)).RankName.."] "..Name(player)))})
			end
			fastaccess[Name(__obj)] = colorstr
		end
	end
end

function MineStars.Player.UpdateNametag(player)
	local meta = player:get_meta()
	local colorstr = (meta:get_string("player_color") ~= "" and meta:get_string("player_color")) or "#FFFFFF"
	player:set_properties({
		nametag = core.colorize(colorstr, "["..((MineStars.XP.GetLevel(MineStars.XP.GetXP(player)) and (not (MineStars.XP.GetLevel(MineStars.XP.GetXP(player)).NotNewbie == false)) and MineStars.XP.GetLevel(MineStars.XP.GetXP(player)).RankName) or "Player").."] "..Name(player))
	})
end

core.register_on_joinplayer(function(player)
	MineStars.Player.IsOnline[player:get_player_name()] = true
	player:hud_set_flags({
		minimap=false,
		minimap_radar=false,
		--[[healthbar=false,
		breathbar=false,--]]
	})
	Controls[Name(player)] = player:get_player_control()
	local meta = player:get_meta()
	local colorstr = (meta:get_string("player_color") ~= "" and meta:get_string("player_color")) or "#FFFFFF"
	player:set_properties({nametag = core.colorize(colorstr, "["..(MineStars.XP.GetLevel(MineStars.XP.GetXP(player)) and MineStars.XP.GetLevel(MineStars.XP.GetXP(player)).RankName.."] "..Name(player)))})
	if meta then
		local str = meta:get_string("player_config")
		if str and str ~= "" then
			local data = core.deserialize(str)
			if data then
				local size = data.size
				if size then
					player:set_properties({
						visual_size = data.size,
						collision_box = {-0.3, 0.0, -0.3, 0.3, (size.y), 0.3},
						eye_height = size.y-0.24
					})
				end
			end
		end
	end
	--print(dump(core.get_player_information(Name(player))))
	--[[
		This is a trick how to detect an multicraft client, its simple:
		5.*** are minetest clients
		0.4** are minetest clients
		2.*** are multicraft clients
	--]]
	local info = core.get_player_information(Name(player))
	if info then
		local version = info.version_string
		if version then
			if version:sub(1, 2) == "2." then
				player:set_multicraft_value(true);
				--print("MULTICRAFT")
				return;
			else
				player:set_multicraft_value(false);
				--print("MINETEST")
				return;
			end
		end
	end
end)

core.register_on_leaveplayer(function(player)
	MineStars.Player.IsOnline[player:get_player_name()] = nil
end)

Sneaking = {}
Controls = {}

core.register_chatcommand("nametag", {
	description = MineStars.Translator("Change your nickname color"),
	params = "<color>",
	func = function(name, params)
		if Player(name) then --Avoid executing command out of the game
			local pstr1 = (params:split(" "))[1]
			if pstr1 then
				MineStars.Player.SetColor(name, pstr1)
				core.chat_send_player(name, "➢"..core.colorize(pstr1, name)..": "..pstr1)
			end
		end
	end
})

core.register_chatcommand("playerinfo", {
	params = "[player]",
	description = MineStars.Translator("Edit your bio or see others bio & info"),
	func = function(n, p)
		if IsOnline(n) then
			local params = p:split(" ")[1]
			if params then
				if IsOnline(params) then
					MineStars.Player.ShowPlayerInfoTo(n, params)
				else
					core.chat_send_player(n, core.colorize("#FF7C7C", "✗"..MineStars.Translator("That player don't exist, or he/she not connected!")))
				end
			else
				MineStars.Player.ShowPlayerInfoTo(n, n)
			end
		else
			return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("You aren't connected!"))
		end
	end
})

local function IncreaseStat(player,statname)
	if not player then return end
	local meta = player:get_meta()
	if meta then
		local int = meta:get_int(statname)
		int = int + 1
		meta:set_int(statname, int)
	end
end

core.register_on_placenode(function(pos, oldnode, player, ext)
	IncreaseStat(player, "nodes_placed")
end)
core.register_on_dignode(function(pos, oldnode, player, ext)
	IncreaseStat(player, "nodes_dug")
end)
core.register_on_dieplayer(function(player, reason)
	local meta = player:get_meta()
	if meta then
		local pos = core.pos_to_string(player:get_pos())
		if pos then
			meta:set_string("death_pos_str", pos)
		end
	end
	if reason.killer and reason.killer:is_player() then
		IncreaseStat(reason.killer, "killed")
		IncreaseStat(player, "dead")
		return
	end
	IncreaseStat(player, "dead")
end)

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "playerinfoeditor" then
		if fields.quit then
			if fields.infeditor then
				local meta = player:get_meta()
				if meta then
					meta:set_string("bio", fields.infeditor)
				end
			end
		end
	end
end)

unified_inventory.register_button("playerinfo", {
	type = "image",
	image = "player_info.png",
	tooltip = MineStars.Translator("Player Info"),
	hide_lite=true,
	action = function(p)
		MineStars.Player.ShowPlayerInfoTo(Name(p), Name(p))
	end
})

---
-- PLAYER ANIMATION (6 HOURS PROGRAMMING THIS SHIT, ENOUGH)
---
--[[
MineStars.Player.PlayerDefaultProperties = {
	mesh = "skinsdb_3d_armor_character_5.b3d",
	textures = {
		"character.png", -- 1
		"blank.png", -- 2
		"blank.png", -- armor
		"blank.png", -- item
	},
	spritediv = {x = 0, y = 0},
	backface_culling = true,
	stepheight = 0.6,
	collisionbox = {
		-0.3,
		0.0,
		-0.3,
		1.75,
		0.3,
	}
}

core.register_on_joinplayer(function(player)
	core.after(0.5, function(player)
		player:set_properties({MineStars.Player.PlayerDefaultProperties})
	end, player)
end)


--]]






--Sound effects for environment
core.register_on_dieplayer(function(player, reason)
	if reason.type == "drown" then
		core.sound_play({name = "engine_drown"}, {to_player = player:get_player_name(), gain = 1})
	end
end)









--[[
core.register_playerevent(function(player, A)
	print(player:get_player_name(), A)
	print(dump(A))
end)

--]]





