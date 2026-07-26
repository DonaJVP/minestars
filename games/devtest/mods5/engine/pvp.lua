--areas
MineStars.PvP = {
	Callbacks = {}
}
function MineStars.PvP.RegisterCallback(f)
	table.insert(MineStars.PvP.Callbacks, f)
end

local function get_bool(str)
	if str == "true" then
		return true
	else
		return false -- lol
	end
end

local S = MineStars.Translator

core.register_on_punchplayer(function(player, hitter, time_from_last_punch, tool_capabilities, dir, damage)
	if MineStars.Shield.States[Name(player)] then
		return true;
	end
	local m1 = player:get_meta()
	local m2 = hitter:get_meta()
	--print("!!")
	if (not m1) or (not m2) then return end
	local c1 = false or get_bool(m1:get_string("can_pvp"))
	local c2 = false or get_bool(m2:get_string("can_pvp"))
	local bo = RunCallbacks(MineStars.PvP.Callbacks, player, hitter, damage, c1, c2)
	if bo then
		return true
	end
	if (not c1) or (not c2) then
		return true
	end
end)

function MineStars.PvP.CanPvP(player)
	return get_bool(player:get_meta():get_string("can_pvp"))
end

local function switcher(bool)
	if not bool then return true end
	if bool then return false end
end

function MineStars.PvP.ChangePvP(player)
	Player(player):get_meta():set_string("can_pvp", tostring((switcher(get_bool(Player(player):get_meta():get_string("can_pvp")))) or "false"))
end

core.register_chatcommand("pvp", {
	params = "",
	description = MineStars.Translator("Change your PvP mode"),
	func = function(name)
		if Player(name) then
			MineStars.PvP.ChangePvP(Player(name))
			return true, core.colorize("lightgreen", "✔"..S("Done!").."\n"..MineStars.Translator("State: @1", tostring(Player(name):get_meta():get_string("can_pvp"))))
		else
			return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Not connected!"))
		end
	end
})

unified_inventory.register_button("pvp", {
	type = "image",
	image = "elementium_sword.png",
	tooltip = MineStars.Translator("PvP"),
	hide_lite=true,
	action = function(p)
		MineStars.PvP.ChangePvP(p)
	end
})
