--Shield [Likely from minecraft but applied here]

MineStars.Function = {
	test = function()
		print("Hello!")
	end
}

MineStars.Shield = {
	Huds = {},
	States = {},
	ShieldsHAS = {},
	ShieldsSTACKS = {},
}

function MineStars.Shield.HasShield(player)
	if Name(player) then
		--Due to a massive lag this will be replaced with 3d_armor support
		--[[local inv = minetest.get_inventory({type="detached", name=Name(player).."_armor"})
		--local inv = Inv(player)
		if not inv then
			return false
		end
		local list = inv:get_list("armor")
		for i, stack in pairs(list) do
			if stack:get_count() == 1 then
				local def = stack:get_definition()
				if def.groups and def.groups.armor_shield and def.groups.armor_shield > 0 then
					return true, stack
				end
			end
		end--]]
		return MineStars.Shield.ShieldsHAS[Name(player)], MineStars.Shield.ShieldsSTACKS[Name(player)]
	end
end

armor:register_on_equip(function(player, index, stack)
	local stack_def = stack:get_definition()
	if stack_def then
		if stack_def.groups and stack_def.groups.armor_shield and stack_def.groups.armor_shield > 0 then
			MineStars.Shield.ShieldsHAS[Name(player)] = true
			MineStars.Shield.ShieldsSTACKS[Name(player)] = stack
		end
	end
end)

armor:register_on_unequip(function(player, index, stack)
	local stack_def = stack:get_definition()
	if stack_def then
		if stack_def.groups and stack_def.groups.armor_shield and stack_def.groups.armor_shield > 0 then
			MineStars.Shield.ShieldsHAS[Name(player)] = false
			MineStars.Shield.ShieldsSTACKS[Name(player)] = ItemStack("")
		end
	end
end)

core.register_on_dieplayer(function(player)
	MineStars.Shield.ShieldsHAS[Name(player)] = nil
end)

function MineStars.Shield.SetStateOfPlayer(player, state_bool)
	
	if state_bool then --enable shield
		if MineStars.Shield.HasShield(player) then
			MineStars.Physics.Set(player, {speed = 0.5, overrideother = true}, true, "FRAMEWORK_ENGINE:Shields", true) 
			--RemovePrivs(player, {"interact"})
			--local armor_groups = player:get_armor_groups()
			--armor_groups.immortal = 1
			--player:set_armor_groups(armor_groups)
			MineStars.Shield.States[Name(player)] = true
			return true
		end
	else
		MineStars.Physics.Remove(player, "FRAMEWORK_ENGINE:Shields")
		--AddPrivs(player, {["interact"] = true})
		--local armor_groups = player:get_armor_groups()
		--armor_groups.immortal = 0
		--player:set_armor_groups(armor_groups)
		MineStars.Shield.States[Name(player)] = false
		return false
	end
	
end

core.register_on_leaveplayer(function(player)
	MineStars.Shield.States[Name(player)] = nil
	MineStars.Shield.ShieldsSTACKS[Name(player)] = ItemStack("")
end)
