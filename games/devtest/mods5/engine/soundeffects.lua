MineStars.SO_Effects = {
	Registered = {
		["drown"] = {sound = "engine_drown", gain = 1},
		["award"] = {sound = "award", gain = 1},
		["quest"] = {sound = "quest", gain = 1}
	}
}

_G["SO_Effects"] = MineStars.SO_Effects

function MineStars.SO_Effects.DoEffect(player, effect)
	if MineStars.SO_Effects.Registered[effect] then
		core.sound_play({
			name = SO_Effects.Registered[effect].sound
		},
		{
			gain = SO_Effects.Registered[effect].gain
		})
	end
end