local savedhuds = {}
hudtable = {}

function update_img(p)
	if p then
		p:hud_change(savedhuds[p:get_player_name()], "text", "heavenly_world_open.png^[opacity:"..hudtable[p:get_player_name()].opaque)
	end
end
--[[
core.register_globalstep(function(dt)
	for pname, data in pairs(hudtable) do
		data.steps = data.steps - dt
		local dont_continue = false
		if data.steps <= 0.2 then
			if data.bool == false then
				if data.opaque < 255 then
					data.opaque = data.opaque + 30
					--print(dump(data), "SUM")
					update_img(core.get_player_by_name(pname))
				else
					data.bool = true
					data.steps = 2
					dont_continue = true
				end
			else
				if data.opaque > 1 then
					data.opaque = data.opaque - 14
					--print(dump(data), "DEL")
					update_img(core.get_player_by_name(pname))
				else
					hudtable[pname] = nil
				end
			end
			if not dont_continue then
				data.steps = 0.2
			end
		end
	end
end)--]]

core.register_on_joinplayer(function(p)
	savedhuds[p:get_player_name()] = p:hud_add({
			hud_elem_type = "image",
			text = "blank.png",
			position = {x = 0.5, y = 0.5},
			offset = {x = 0, y = 0},
			scale = {x = 1, y = 1},
		})
	core.after(2, function(p)
		
		hudtable[p:get_player_name()] = {opaque = 0, steps = 0, bool = false}
	end, p)
end)