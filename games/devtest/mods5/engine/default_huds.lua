MineStars.DefaultHuds = {
	Huds = {
		--Logiki = {Breath=12,Hp=13,Food=14}
	},
	BackgroundBreath = {},
	BackgroundFood = {},
}

local hp_offset = {x=-220,y=-86}
local food_offset = {x=20,y=-86}
local breath_offset = {x=-85,y=-130}

core.register_on_joinplayer(function(player)
	--HP
	--[[player:hud_add({
		hud_elem_type = "statbar",
		position = {x=0.5,y=1},
		scale = {x=1,y=1},
		text = "heart_bg.png",
		number = 20,
		alignment = {x=-1,y=-1},
		offset = hp_offset,
		direction = 0,
		size = {x = 20, y = 20},
	})--]]
	--Food
	MineStars.DefaultHuds.BackgroundFood[Name(player)] = player:hud_add({
		hud_elem_type = "statbar",
		position = {x=0.5,y=1},
		scale = {x=1,y=1},
		text = "hunger_bg.png",
		number = 20,
		alignment = {x=-1,y=-1},
		offset = food_offset,
		direction = 0,
		size = {x = 20, y = 20},
	})
	--[[Breath
	MineStars.DefaultHuds.BackgroundBreath[Name(player)] = player:hud_add({
		hud_elem_type = "statbar",
		position = {x=0.5,y=1},
		scale = {x=1,y=1},
		text = "blank.png",
		number = 20,
		alignment = {x=-1,y=-1},
		offset = breath_offset,
		direction = 0,
		size = {x = 20, y = 20},
	})--]]
	MineStars.DefaultHuds.Huds[Name(player)] = {
		--[[Hp = player:hud_add({
			hud_elem_type = "statbar",
			position = {x=0.5,y=1},
			text = "heart.png",
			number = player:get_hp(),
			alignment = {x=-1,y=-1},
			offset = hp_offset,
			direction = 0,
			size = {x = 20, y = 20},
		}),--]]
		Food = player:hud_add({
			hud_elem_type = "statbar",
			position = {x=0.5,y=1},
			text = "hunger.png",
			number = MineStars.Food.GetLevel(player),
			alignment = {x=-1,y=-1},
			offset = food_offset,
			direction = 0,
			size = {x = 20, y = 20},
		}),
		--[[Breath = player:hud_add({
			hud_elem_type = "statbar",
			position = {x=0.5,y=1},
			text = "blank.png",
			number = 0,
			alignment = {x=-1,y=-1},
			offset = breath_offset,
			direction = 0,
			size = {x = 20, y = 20},
		})--]]
	}
	
end)

core.hud_replace_builtin("breath", {
	hud_elem_type = "statbar",
	name = "breath",
	type = "statbar",
	position = {x=0.5,y=1},
	text = "breath.png",
	text2 = "breath_bg.png",
	number = 20,
	item = 20,
	alignment = {x=-1,y=-1},
	offset = breath_offset,
	direction = 0,
	size = {x = 20, y = 20},
})

core.hud_replace_builtin("health", {
	hud_elem_type = "statbar",
	name = "health",
	type = "statbar",
	position = {x=0.5,y=1},
	text = "heart.png",
	text2 = "heart_bg.png",
	number = 20,
	item = 20,
	alignment = {x=-1,y=-1},
	offset = hp_offset,
	direction = 0,
	size = {x = 20, y = 20},
})


function MineStars.DefaultHuds.UpdateHungerLevel(player, level)
	player:hud_change(MineStars.DefaultHuds.Huds[player:get_player_name()].Food, "number", level)
	if level < 10 then
		player:hud_change(MineStars.DefaultHuds.BackgroundFood[player:get_player_name()], "text", "hunger_bg_extreme.png")
	else
		player:hud_change(MineStars.DefaultHuds.BackgroundFood[player:get_player_name()], "text", "hunger_bg.png")
	end
end
--[[
core.register_on_player_hpchange(function(player, hp_change, reason)
	player:hud_change(MineStars.DefaultHuds.Huds[player:get_player_name()].Hp, "number", player:get_hp())
end)
--]]





