MineStars.Vanish = {
	Active = {},
}

local function update_visuals(name)
	local player = Player(name)
	if player then
		if MineStars.Vanish.Active[name] then
			player:set_properties({
				visual_size = vector.new(0,0,0),
				nametag = " ",
				is_visible = false,
				pointable = false,
			})
			if MineStars.HeadsObjects[name] then
				MineStars.HeadsObjects[name]:set_properties({
					is_visible = false
				})
			end
		else
			player:set_properties({
				visual_size = vector.new(1,1,1),
				nametag = core.colorize(MineStars.Player.GetColor(name), name),
				pointable = true,
				is_visible = true,
			})
			if MineStars.HeadsObjects[name] then
				MineStars.HeadsObjects[name]:set_properties({
					is_visible = true
				})
			end
		end
	end
end

core.register_chatcommand("vanish", {
	params = "",
	privs = {server = true},
	description = "Vanish",
	func = function(name)
		if Player(name) then
			if not MineStars.Vanish.Active[name] then
				MineStars.Vanish.Active[name] = true
				update_visuals(name)
				return true, core.colorize("lightgreen", C_..MineStars.Translator("Done!"))
			else
				MineStars.Vanish.Active[name] = nil
				update_visuals(name)
				return true, core.colorize("lightgreen", C_..MineStars.Translator("Done!"))
			end
		else
			return true, core.colorize("#FF7C7C", X..MineStars.Translator("Must be connected!"))
		end
	end,
})