core.register_craftitem(":totem:totem", {
    description = "Totem",
    inventory_image = "totem.png", 
    on_place = function(itemstack, placer, pointed_thing)
        --return itemstack
	--Get offhand value if inst nil
	local stack = Inv(placer):get_stack("offhand", 1)
	if stack:get_count() <= 0 then
		Inv(placer):set_stack("offhand", 1, itemstack)
		return ItemStack("totem:totem 0")
	end
	return itemstack
    end,
	stack_max = 1,
	max_stack = 1,
})

local func = core.show_death_screen
function core.show_death_screen(player, _)
	local inv = player and Inv(player)
	if inv then
		if inv:contains_item("main", "totem:totem") or inv:contains_item("offhand", "totem:totem") then
			return false
		end
		return func(player, _) or true
	else
		return func(player, _) or true
	end
end


MineStars.Dead = {
	Cache = {}
}
core.register_on_dieplayer(function(player)
    local inv = player:get_inventory()
    if inv:contains_item("main", "totem:totem") or inv:contains_item("offhand", "totem:totem") then
        inv:remove_item("main", "totem:totem")
        player:set_hp(20) 
         minetest.sound_play("totem", {
            to_player = player:get_player_name(),
            gain = 2, 
        })
	MineStars.Dead.Cache[Name(player)] = true
        local pos = player:get_pos()
        minetest.add_particlespawner({
            amount = 15, 
            time = 1.5,
            minpos = {x = pos.x - 1, y = pos.y, z = pos.z - 1},
            maxpos = {x = pos.x + 1, y = pos.y + 2, z = pos.z + 1},
            minvel = {x = -1, y = 1, z = -1},
            maxvel = {x = 1, y = 2, z = 1},
            minacc = {x = 0, y = 0, z = 0},
            maxacc = {x = 0, y = 0, z = 0},
            minexptime = 2,
            maxexptime = 3,
            minsize = 1,
            maxsize = 3,
            texture = "totem.png", 
        })
	print("LAPUTA")
        return true 
    end
end)

core.register_on_respawnplayer(function(player)
	if MineStars.Dead.Cache[Name(player)] then
		MineStars.Dead.Cache[Name(player)] = nil
		return true
	end
end)
