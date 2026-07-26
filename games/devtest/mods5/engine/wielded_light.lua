--WIELDED LIGHTHTHTTTTTT

MineStars.WieldedLight = {
	ActualNodes = {},
	DefaultNodeOfPos = {},
	RegisteredInventories = {
		--[[
		{InvListName = "headlight", Slot = 1}
		--]]
	}
}

function ReturnRealPosForLight(player_pos)
	player_pos = vector.apply(player_pos, math.round)
	return {x = player_pos.x, y = player_pos.y + 1, z = player_pos.z}
end

function MineStars.WieldedLight.RegisterInventoryName(invlistname, slot) -- select ever
	slot = slot or 1
	table.insert(MineStars.WieldedLight.RegisteredInventories, {InvListName = invlistname, Slot = slot})
end

do
	for i = 1, 15 do
		core.register_node("engine:light_"..i, {
			description = "Light "..i,
			light_source = i,
			pointable = false,
			sunlight_propagates = true,
			walkable = false,
			tiles = {"blank.png"},
			drawtype = "glasslike",
			on_timer = function(pos)
				core.set_node(pos, {name = "air"})
			end,
			groups = {not_in_creative_inventory=1},
			on_blast = function() end
		})
	end
end

local already = {}
local queues = {}
function MineStars.WieldedLight.SetLightNode(pos, lvl)--set(0.5)
	if core.get_node(pos).name and core.get_node(pos).name == "air" and (not core.get_node(pos).name:match"engine:light") then
		core.swap_node(pos, {name = "engine:light_"..lvl})
		core.get_node_timer(pos):set(0.8, 0.1)
	else
		if core.get_node(pos).name and core.get_node(pos).name ~= "air" and core.get_node(pos).name:match"engine:light" then
			core.get_node_timer(pos):set(0.8, 0.1)
		end
	end
end

local ms = 0
core.register_playerstep(function(dt, playersname)
	for _, name in pairs(playersname) do
		local p = core.get_player_by_name(name)
		if p and not MineStars.Vanish.Active[name] then
			local item = p:get_wielded_item()
			local offhanditm = return_inv_or_create(p, true):get_stack("offhand", 1)
			--print(offhanditm:get_name())
			local pos = ReturnRealPosForLight(p:get_pos())
			local Force = false
			if item and item:get_count() > 0 then
				local def = item:get_definition()
				local light = def.light_source
				if light and light > 4 then
					MineStars.WieldedLight.SetLightNode(pos, light)
				else
					Force = true
				end
			end
			if Force or (offhanditm and offhanditm:get_count() > 0) then
				local def = offhanditm:get_definition()
				local light = def.light_source
				if light and light > 4 then
					MineStars.WieldedLight.SetLightNode(pos, light)
				end
			end
		end
	end
end)






