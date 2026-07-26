--Offhand
MineStars.Offhand = {
	Huds = {
		--[[
		Logiki = {
			WearBarBg = 0x0,
			WearBar = 0x1,
			Slot = 0x2,
			Item = 0x3,
			ItemCount = 0x4,
		}
		--]]
	},
	Items = {
		--[[
		OnUse = func
		OnSecondaryUse = func
		OnPlace = func
		--]]
	},
	Cache = {},
	OtherOnUse = {},
	OtherOnPlace = {},
	OtherOnSecondaryUse = {},
}

--prefer to use detached_inventory for those juicy callbacks
--core.create_detached_inventory(name, callbacks, [player_name])
--[[
	{
    allow_move = function(inv, from_list, from_index, to_list, to_index, count, player),
    -- Called when a player wants to move items inside the inventory.
    -- Return value: number of items allowed to move.

    allow_put = function(inv, listname, index, stack, player),
    -- Called when a player wants to put something into the inventory.
    -- Return value: number of items allowed to put.
    -- Return value -1: Allow and don't modify item count in inventory.

    allow_take = function(inv, listname, index, stack, player),
    -- Called when a player wants to take something out of the inventory.
    -- Return value: number of items allowed to take.
    -- Return value -1: Allow and don't modify item count in inventory.

    on_move = function(inv, from_list, from_index, to_list, to_index, count, player),
    on_put = function(inv, listname, index, stack, player),
    on_take = function(inv, listname, index, stack, player),
    -- Called after the actual action has happened, according to what was
    -- allowed.
    -- No return value.
}
--]]

local function on_update(inv, _, _, s_, P_, _, P__)
	if type(P_) == "userdata" then
		MineStars.UpdateHud(P_)
		MineStars.Offhand.UpdatePhysicalHand(P_)
		Inv(P_):set_stack("offhand", 1, s_)
	elseif P__ then
		MineStars.UpdateHud(P__)
		MineStars.Offhand.UpdatePhysicalHand(P__)
		--Inv(P__):set_stack("offhand", s_) --not defined!
	end
end

MineStars.Offhand.QueueToUpdate = {
	--[[
	Logiki = {
		Pass1 = false,
		Pass2 = false,
	}
	--]]
}

local function update_inv(player)
	local stack = return_inv_or_create(player):get_stack("offhand", 1)
	Inv(player):set_stack("offhand", 1, stack)
end

--func_c = {}

function return_inv_or_create(player, ONLYINV)
	local is_created = not core.get_inventory({type = "detached", name = Name(player).."_offhand"})
	local inv = core.get_inventory({type = "detached", name = Name(player).."_offhand"}) or core.create_detached_inventory(Name(player).."_offhand", {
		on_move = on_update,
		on_put = function(inv, listname, index, stack, player)
			--[[MineStars.UpdateHud(player, stack)
			MineStars.Offhand.UpdatePhysicalHand(player, stack)
			Inv(player):set_stack("offhand", 1, stack)
			MineStars.UpdateHud(player)--]]
			core.after(0.1, MineStars.Offhand.UpdatePhysicalHand, player)
			core.after(0.2, update_inv, player)
			MineStars.UpdateHud(player)
		end,
		--allow_take = function(inv, listname, index, stack, player)
			--func_c[player:get_player_name()] = stack --copy for other time
		--	return stack:get_count()
		--end,
		on_take = function(inv, listname, index, stack, player)
			--make sure to update later the stack on the player inventory (storing data)
			core.after(0.1, MineStars.Offhand.UpdatePhysicalHand, player)
			core.after(0.2, update_inv, player)
			MineStars.UpdateHud(player)
		end,
	})
	--if is_created then
	--if not ONLYINV then
		inv:set_size("offhand", 1)
	--end
	--end
	return inv
end

core.register_on_joinplayer(function(player)
	--JIT load
	local inv__ = return_inv_or_create(player)
	--savings
	local inv = player:get_inventory()
	inv:set_size("offhand", 1)
	if inv__:get_stack("offhand", 1):get_count() <= 0 then
		inv__:set_stack("offhand", 1, inv:get_stack("offhand", 1))
	end
	--Huds
	
	MineStars.Offhand.Huds[Name(player)] = {
		Slot = player:hud_add({
			hud_elem_type = "image",
			position = {x=0.5,y=1},
			offset = {x=-320,y=-32},
			scale = {x=2.75,y=2.75},
			text = "blank.png",
			z_index = 0,
		}),
		Item = player:hud_add({
			hud_elem_type = "image",
			position = {x=0.5,y=1},
			offset = {x=-320,y=-32},
			scale = {x=0.4,y=0.4},
			text = "blank.png",
			z_index = 1,
		}),
		WearBarBg = player:hud_add({
			hud_elem_type = "image",
			position = {x=0.5,y=1},
			offset = {x=-320,y=-13},
			scale = {x = 40, y = 3},
			text = "blank.png",
			z_index = 2,
		}),
		WearBar = player:hud_add({
			hud_elem_type = "image",
			position = {x=0.5,y=1},
			offset = {x=-320,y=-13},
			scale = {x = 10, y = 3},
			text = "blank.png",
			z_index = 3,
		}),
		ItemCount = player:hud_add({
			hud_elem_type = "text",
			position = {x=0.5,y=1},
			offset = {x=-298,y=-18},
			alignment = {x=-1,y=0},
			text = "",
			z_index = 4,
			number = 0xFFFFFF
		})
	}
	core.after(0.4, MineStars.UpdateHud, player)
	core.after(0.6, MineStars.Offhand.SetPhysicalHand, player)
end)

core.register_on_leaveplayer(function(p)
	MineStars.Offhand.Huds[Name(p)] = nil
end)

if bones then
	table.insert(bones.player_inventory_lists, "offhand")
end

local function OffHandUse(secondary, mainhand_stack, player, pointed_thing, ...)
	local offhand_stack = player:get_wielded_item()
	local offhand_def = offhand_stack:get_definition()
	local use = offhand_def.on_place
	if secondary then use = offhand_def.on_secondary_use or do_nothing end
	local modified_stack = use(offhand_stack, player, pointed_thing, ...)
	if modified_stack == nil then modified_stack = offhand_stack end
	return mainhand_stack
end

--Overriode all nodes existing in the game.
local function srfodmd(player, pointed_thing, itemstack, typo, item_type)
	--print("srfodmd", dump(pointed_thing))
	--[[--00RIghtlcick
	local pos = pointed_thing and pointed_thing.under or nil
	print(dump(pos))
	local node = pos and core.get_node(pos)
	print(dump(node))
	if node then
		print(1)
		if node.name and (node.name ~= "ignore") and core.registered_items[node.name] then	
			print(2)
			if core.registered_items[node.name].on_rightclick then
				--on_rightclick = function(pos, node, clicker, itemstack, pointed_thing)
				core.registered_items[node.name].on_rightclick(pos, node, player, itemstack, pointed_thing)
				print("tttrrr")
				return itemstack
			end
		end
	end--]]
	
	--LEADS []
	local controls = player:get_player_control()
	local node = pointed_thing.under and minetest.get_node_or_nil(pointed_thing.under)
	if node and leads and leads.is_knottable(node.name) then
		if leads.knot(placer, pointed_thing.under) then
			return nil
		end
	end

	local name = itemstack:get_name()
	if typo == "on_place" then
		if item_type == "node" then
			
		else -- either tool, craftitem, etc.... [mental note]
			if MineStars.Offhand.OtherOnPlace[name] then
				local itm = MineStars.Offhand.OtherOnPlace[name](itemstack, player, pointed_thing) --onplace
				local itm_n = (itm and itm:get_name()) or ""
				if itm and ((itm_n ~= itemstack:get_name()) or itm:get_count() ~= itemstack:get_count()) then
					return itm
				--elseif itm_n == itemstack:get_name() then
				--	return itemstack
				else
					--proccess hand
					local inv = return_inv_or_create(player)
					if inv then
						local offhand_itm = inv:get_stack("offhand", 1)
						if not offhand_itm:is_empty() then
							local def = offhand_itm:get_definition()
							if def then
								local func = MineStars.Offhand.Items[offhand_itm:get_name()].OnPlace
								if func then
									local stack = func(offhand_itm, player, pointed_thing)
									inv:set_stack("offhand", 1, stack or offhand_itm)
									if not ((not stack) or (offhand_itm:get_name() == stack:get_name() and offhand_itm:get_count() == stack:get_count())) then --equals:offhand_itm==stack
										local xd = core.item_place(offhand_itm, player, pointed_thing)
										inv:set_stack("offhand", 1, xd)
										Inv(player):set_stack("offhand", 1, xd)
										MineStars.Offhand.Cache[Name(player)] = xd
									end
									MineStars.UpdateHud(player)
								else
									local idkhowtonamethis = core.item_place(offhand_itm, player, pointed_thing)
									inv:set_stack("offhand", 1, idkhowtonamethis)
									Inv(player):set_stack("offhand", 1, idkhowtonamethis)
									MineStars.Offhand.Cache[Name(player)] = idkhowtonamethis
								end
								MineStars.UpdateHud(player)
							end
						end
					end
					if itm_n == itemstack:get_name() then
						return itemstack
					end
				end
			else
				local inv = return_inv_or_create(player)
				if inv then
					local offhand_itm = inv:get_stack("offhand", 1)
					if not offhand_itm:is_empty() then
						local def = offhand_itm:get_definition()
						if def then
							local func = MineStars.Offhand.Items[offhand_itm:get_name()].OnPlace
							if func then
								local stack = func(offhand_itm, player, pointed_thing)
								inv:set_stack("offhand", 1, stack or offhand_itm)
								if not ((not stack) or (offhand_itm:get_name() == stack:get_name() and offhand_itm:get_count() == stack:get_count())) then
									local __e = core.item_place(offhand_itm, player, pointed_thing)
									inv:set_stack("offhand", 1, __e)
									Inv(player):set_stack("offhand", 1, __e)
								end
								MineStars.UpdateHud(player)
							else
								local ____ = core.item_place(offhand_itm, player, pointed_thing)
								inv:set_stack("offhand", 1, ____)
								Inv(player):set_stack("offhand", 1, ____)
								MineStars.UpdateHud(player)
							end
						end
					end
				end
				return itemstack
			end
		end
	elseif typo == "on_use" then
		if MineStars.Offhand.OtherOnUse[name] then
			local itm = MineStars.Offhand.OtherOnUse[name](itemstack, player, pointed_thing) --onplace
			local itm_n = (itm and itm:get_name()) or ""
			if itm_n ~= itemstack:get_name() then
				return itm
			else
				--proccess hand
				local inv = return_inv_or_create(player)
				if inv then
					local offhand_itm = inv:get_stack("offhand", 1)
					if not offhand_itm:is_empty() then
						local def = offhand_itm:get_definition()
						if def then
							local func = MineStars.Offhand.Items[offhand_itm:get_name()].OnUse
							if func then
								local stack = func(offhand_itm, player, pointed_thing)
								inv:set_stack("offhand", 1, stack or offhand_itm)
								MineStars.UpdateHud(player)
							end
						end
					end
				end
			end
			return itemstack
		else
			local inv = return_inv_or_create(player)
			if inv then
				local offhand_itm = inv:get_stack("offhand", 1)
				if not offhand_itm:is_empty() then
					local def = offhand_itm:get_definition()
					if def then
						local func = MineStars.Offhand.Items[offhand_itm:get_name()].OnUse
						if func then
							local stack = func(offhand_itm, player, pointed_thing)
							inv:set_stack("offhand", 1, stack or offhand_itm)
							MineStars.UpdateHud(player)
						end
					end
				end
			end
			return itemstack
		end
	elseif typo == "on_secondary_use" then
		if MineStars.Offhand.OtherOnSecondaryUse[name] then
			local itm = MineStars.Offhand.OtherOnSecondaryUse[name](itemstack, player, pointed_thing)
			local itm_n = (itm and itm:get_name()) or ""
			if itm_n ~= itemstack:get_name() then
				return itm
			else
				--proccess hand
				local inv = return_inv_or_create(player)
				if inv then
					local offhand_itm = inv:get_stack("offhand", 1)
					if not offhand_itm:is_empty() then
						local def = offhand_itm:get_definition()
						if def then
							local func = MineStars.Offhand.Items[offhand_itm:get_name()].OnSecondaryUse
							if func then
								local stack = func(offhand_itm, player, pointed_thing)
								inv:set_stack("offhand", 1, stack or offhand_itm)
								MineStars.UpdateHud(player)
							end
						end
					end
				end
			end
			return itemstack
		else
			local inv = return_inv_or_create(player)
			if inv then
				local offhand_itm = inv:get_stack("offhand", 1)
				if not offhand_itm:is_empty() then
					local def = offhand_itm:get_definition()
					if def then
						local func = MineStars.Offhand.Items[offhand_itm:get_name()].OnSecondaryUse
						if func then
							local stack = func(offhand_itm, player, pointed_thing)
							inv:set_stack("offhand", 1, stack or offhand_itm)
							MineStars.Offhand.Cache[Name(player)] = stack or offhand_itm
							MineStars.UpdateHud(player)
						end
					end
				end
			end
			return itemstack
		end
	end
end

local subtable = {}
local skippable = {}

core.register_on_mods_loaded(function()
	for name, def in pairs(core.registered_items) do
		--Save all functions!
		MineStars.Offhand.Items[name] = {
			OnPlace = def.on_place or (function(i) return i end),
		}
	end
	for name, def in pairs(core.registered_nodes) do
		skippable[name] = true
	end
	for name, def in pairs(core.registered_items) do
		--.log("action", "Viewed item: "..name)
		if not skippable[name] then --already registered
			local defunc = table.copy(def)
			if def.on_place then
				MineStars.Offhand.OtherOnPlace[name] = defunc.on_place
				--print("handled")
			end
			core.registered_items[name].on_place = function(itemstack, user, pointed_thing)
				local pos = pointed_thing and pointed_thing.under or nil
				--print(dump(pos))
				local node = pos and core.get_node(pos)
				--print(dump(node))
				if node then
					--print(1)
					if node.name and (node.name ~= "ignore") and core.registered_items[node.name] then	
						--print(2)
						if core.registered_items[node.name].on_rightclick then
							--on_rightclick = function(pos, node, clicker, itemstack, pointed_thing)
							core.registered_items[node.name].on_rightclick(pos, node, user, itemstack, pointed_thing)
							--print("tttrrr")
							return itemstack
						end
					end
				end
				return srfodmd(user, pointed_thing, itemstack, "on_place", "")
			end
		end
	end
end)



function MineStars.Offhand.GetHand(player)
	return (MineStars.Offhand.Cache[Name(player)] or (((function(player) MineStars.Offhand.Cache[Name(player)] = return_inv_or_create(player):get_stack("offhand", 1); return true end)(player) or true) and return_inv_or_create(player):get_stack("offhand", 1)) or return_inv_or_create(player):get_stack("offhand", 1)) --HAHAHAH
end

local function get_pointed_thing(player)
	if player:get_pos() then
		MineStars.Offhand.Cache[Name(player)] = MineStars.Offhand.GetHand(player)
		--- determine pointing range
		local range = 5
		-- adjust player position
		local pos1 = player:get_pos()
		pos1 = vector.add(pos1, player:get_eye_offset())
		pos1.y = pos1.y + (player:get_properties()).eye_height
		-- cast ray from player's eyes
		local pos2 = vector.add(pos1, vector.multiply(player:get_look_dir(), range))
		local ray = Raycast(pos1, pos2, false, false)
		-- iterate through passed nodes and determine if pointable
		local result = nil
		for pointed_thing in ray do
			if pointed_thing.type == "node" then
				local node = minetest.get_node(pointed_thing.under)
				local nodedef = minetest.registered_nodes[node.name]
				if nodedef and nodedef.pointable ~= false then
					result = pointed_thing
					break
				end
			else
				result = nil
				break
			end
		end
		return result
	else
		return false
	end
end

local function OnUse(player, pointed_thing, isfood)
	--if (not pointed_thing) or (not isfood) then return end
	---print("SDJOPIDJ")
	--print("exEC+")
	if (not pointed_thing) and not isfood then
		return
	end
	local wielded_item = player:get_wielded_item()
	if wielded_item and wielded_item:get_count() > 0 then
		--if not wielded_item:get_definition().on_use then
		if not core.registered_is_use[wielded_item:get_name()] then
			--print("checkpoint1")
			local inv = return_inv_or_create(player)
			if inv then
				local offhand_itm = inv:get_stack("offhand", 1)
				if not offhand_itm:is_empty() then
					local def = offhand_itm:get_definition()
					if def then
						local func = def.on_use
						if func then
							--print("exec onusr")
							local stack = func(offhand_itm, player, pointed_thing)
							inv:set_stack("offhand", 1, stack or offhand_itm)
							MineStars.Offhand.Cache[Name(player)] = stack or offhand_itm
							Inv(player):set_stack("offhand", 1, stack or offhand_itm)
							MineStars.UpdateHud(player, stack or offhand_itm)
						end
					end
				end
			end
		end
	else
		local inv = return_inv_or_create(player)
		if inv then
			local offhand_itm = inv:get_stack("offhand", 1)
			if not offhand_itm:is_empty() then
				local def = offhand_itm:get_definition()
				if def then
					local func = def.on_use
					if func then
						local stack = func(offhand_itm, player, pointed_thing)
						inv:set_stack("offhand", 1, stack or offhand_itm)
						MineStars.Offhand.Cache[Name(player)] = stack or offhand_itm
						MineStars.UpdateHud(player, stack or offhand_itm)
					end
				end
			end
		end
	end
	MineStars.UpdateHud(player)
end

local function OnPlace(player, pointed_thing)
	if not pointed_thing then return end
	local wielded_item = player:get_wielded_item()
	if wielded_item and wielded_item:get_count() > 0 then	
		--print(dump(wielded_item:get_definition()))
		--print(dump(wielded_item:get_definition()))
		if (not wielded_item:get_definition().on_place) and wielded_item:get_definition().type ~= "node" then
			local inv = return_inv_or_create(player)
			if inv then
				local offhand_itm = inv:get_stack("offhand", 1)
				if not offhand_itm:is_empty() then
					local def = offhand_itm:get_definition()
					if def then
						local func = def.on_place
						if func then
							local stack = func(offhand_itm, player, pointed_thing)
							if not ((not stack) or (offhand_itm:get_name() == stack:get_name() and offhand_itm:get_count() == stack:get_count())) then
								local thing = core.item_place(offhand_itm, player, pointed_thing)
								inv:set_stack("offhand", 1, thing)
								MineStars.UpdateHud(player, thing)
								MineStars.Offhand.Cache[Name(player)] = thing
							end
							--MineStars.UpdateHud(player)
						else
							local temple = core.item_place(offhand_itm, player, pointed_thing)
							MineStars.Offhand.Cache[Name(player)] = temple
							inv:set_stack("offhand", 1, temple)
							MineStars.UpdateHud(player, temple)
							
						end
					end
				end
			end
		elseif wielded_item:get_definition().on_place then
			--this already executed by player
		end
	else
		local inv = return_inv_or_create(player)
		if inv then
			local offhand_itm = inv:get_stack("offhand", 1)
			if not offhand_itm:is_empty() then
				local def = offhand_itm:get_definition()
				if def then
					local func = def.on_place
					if func then
						local stack = func(offhand_itm, player, pointed_thing)
						if not ((not stack) or (offhand_itm:get_name() == stack:get_name() and offhand_itm:get_count() == stack:get_count())) then
							local thing = core.item_place(offhand_itm, player, pointed_thing)
							inv:set_stack("offhand", 1, thing)
							MineStars.UpdateHud(player, thing)
							MineStars.Offhand.Cache[Name(player)] = thing
						end
						MineStars.UpdateHud(player)
					else
						local temple = core.item_place(offhand_itm, player, pointed_thing)
						MineStars.Offhand.Cache[Name(player)] = temple
						inv:set_stack("offhand", 1, temple)
						--MineStars.UpdateHud(player)
						MineStars.UpdateHud(player, temple)
					end
				end
			end
		end
	end
	MineStars.UpdateHud(player)
end

itemisfood = {}

--idk how minetest handles "on_use" on "on_secondary_use" when theres food in the hand....
local function is_food_item(player, inv, funcn)
	inv = inv or return_inv_or_create(player)
	if inv then
		--print(1)
		local stack = (not funcn) and inv:get_stack("offhand", 1) or nil
		if funcn then
			stack = player[funcn](player) -- acts like a table.
		end
		if itemisfood[stack:get_name()] ~= nil then return itemisfood[stack:get_name()] end --don't do big script again if it already in cache
		print2(2, stack:get_name(), stack:get_count())
		if stack:get_count() > 0 then
		--	print(3)
			local def = stack:get_definition()
			if def.groups.eatable or def.groups.food_berry or def.groups.food_apple then --farming and default [blueberry, apple]
				itemisfood[stack:get_name()] = true
				--print(4)
				return true
			end
		end
	end
	--print("DJAOPDJO")
	return false
end

local function OnSec(player, pointed_thing)
	--print("checkpoint PRSTDA>S 611")
	local wielded_item = player:get_wielded_item()
	if wielded_item and wielded_item:get_count() > 0 then
		--print(1)
		if (not wielded_item:get_definition().on_secondary_use) and not (is_food_item(player, Inv(player), "get_wielded_item")) then
		--print("ACCEPT _> 616")
			local inv = return_inv_or_create(player)
			if inv then
			--print(3)
				local offhand_itm = inv:get_stack("offhand", 1)
				if not offhand_itm:is_empty() then
					local def = offhand_itm:get_definition()
					if def then
						local func = def.on_secondary_use
						--print(func)
						if func then
							--print("exec")
							local stack = func(offhand_itm, player, pointed_thing)
							MineStars.Offhand.Cache[Name(player)] = stack or offhand_itm
							inv:set_stack("offhand", 1, stack or offhand_itm)
							Inv(player):set_stack("offhand", 1, stack or offhand_itm)
							--MineStars.UpdateHud(player)
						end
					end
				end
			end
		end
	else
		local inv = return_inv_or_create(player)
		if inv then
			local offhand_itm = inv:get_stack("offhand", 1)
			if not offhand_itm:is_empty() then
				local def = offhand_itm:get_definition()
				if def then
					local func = def.on_secondary_use
					if func then
						local stack = func(offhand_itm, player, pointed_thing)
						inv:set_stack("offhand", 1, stack or offhand_itm)
						MineStars.Offhand.Cache[Name(player)] = stack or offhand_itm
						Inv(player):set_stack("offhand", 1, stack or offhand_itm)
					end
				end
			end
		end
	end
	MineStars.UpdateHud(player)
end
--[[
local on_place = {}
local on_use = {}
local on_sec = {}
local ms = 0
core.register_globalstep(function(dt, players, controls_)
	--print(dump(controls_))
	MineStars.Lag = dt
	--ms = ms + dt
	--if ms >= 0.1 then
		--ms = 0
		for name, controls in pairs(controls_) do
			local p = controls.player
			if p then
				--MineStars.UpdateHud(p)
				--print(dump(p:get_player_control_bits()))
				--print("ipate")
				if controls then
					if controls.dig then
						local pointed_thing = get_pointed_thing(p)
						if not pointed_thing then
							if not on_sec[name] then
								on_sec[name] = true
								OnSec(p, pointed_thing)
							end
						else
							if not on_use[name] then
								on_use[name] = true
								OnUse(p, pointed_thing)
							end
						end
					else
						if on_use[name] then
							on_use[name] = nil
						end
						if on_sec[name] then
							on_sec[name] = nil
						end
					end
					if controls.place then
						local pointed_thing = get_pointed_thing(p)
					--	if (not pointed_thing) or (not pointed_thing.above) then
							if not on_place[name] then
								on_place[name] = true
								OnPlace(p, pointed_thing)
							end
						--end
					else
						if on_place[name] then
							on_place[name] = nil
						end
					end
				end
			end
		end
	--end
end)

core.register_on_punchnode(function(pos, node, puncher, pointed_thing)
	print(dump(pos))
end)

core.register_playerevent(function(_, a, b, c)
	print("EVENT", _, a, b, c)
end)

core.register_on_item_eat(function(_)
	print("FUNC")
end)


	print("AAA", a, b , c)
	print(dump(b))
end)

core.register_on_modchannel_message(function()
	print("MODCHANNEL")
end)
--]]

--MODIFY ITEMPLACE FFOR PLACING THINGS OF THE OFFHAND

--on script/cpp_api/s_env.cpp and network/serverpackethandler.cpp
local pressed = {}
function core.func_on_click(player, dig, place)
	--print("click: "..tostring(dig))
	--print("!")
	local name = player:get_player_name()
	if dig then
		--if not pressed[name] then
		--	pressed[name] = true
			local pointed_thing = get_pointed_thing(player)
			--Handle on_secondary_use as on_use if the item is food, idk how this works
			if is_food_item(player) then
				OnUse(player, pointed_thing or {}, true);
				--print("food!")
				return; --don't exec other things
			end
		
			
			if not pointed_thing then
				OnSec(player, pointed_thing)
				--print("onsec")
				MineStars.Offhand.UpdatePhysicalHand(player)
			else
				OnUse(player, pointed_thing)
				--print("onuse")
				MineStars.Offhand.UpdatePhysicalHand(player)
			end
			
			
		--else
		--	pressed[name] = nil
		--end
	end
end

local oisu = core.item_secondary_use
function core.item_secondary_use(itemstack, user, pointed_thing)
	OnSec(user, pointed_thing, true)
	--print("ejecute")
	oisu(itemstack, user, pointed_thing)
end

function rgb_to_hex(r, g, b)
	return string.format("%02x%02x%02x", r, g, b)
end

local function get_tile_name(tiledef)
	if type(tiledef) == "table" then return tiledef.name end
	return tiledef
end

local max_offhand_px = 128
function ReturnIcon(itemdef)
	if itemdef.inventory_image and itemdef.inventory_image ~= "" then
		return itemdef.inventory_image .. "^[resize:" .. max_offhand_px .. "x" .. max_offhand_px
	elseif not itemdef.tiles or not itemdef.tiles[1] then
			return "blank.png^[resize:" .. max_offhand_px .. "x" ..max_offhand_px
	end
	local tiles = {
		get_tile_name(itemdef.tiles[1]) .. "^[resize:" .. max_offhand_px .. "x" .. max_offhand_px,
		get_tile_name(itemdef.tiles[3] or itemdef.tiles[1]) .. "^[resize:" .. max_offhand_px .. "x" .. max_offhand_px,
		get_tile_name(itemdef.tiles[5] or itemdef.tiles[3] or itemdef.tiles[1]) .. "^[resize:" .. max_offhand_px .. "x" .. max_offhand_px
	}
	for i, tile in pairs(tiles) do
		if (type(tile) == "table") then
			tiles[i] = tile.name
		end
	end
	local textures = table.concat(tiles, "{")
	return "[inventorycube{" .. (textures:gsub("%^", "&")) .. "^[resize:" .. max_offhand_px .. "x" ..max_offhand_px
end

function MineStars.UpdateHud(p, ITM)
	--print(Name(p), p)
	core.after(0.2, function(p, ITM)
		local item = ITM or return_inv_or_create(p):get_stack("offhand", 1)
		--print(dump(item:get_name(), item:get_count()))
		if MineStars.Offhand.Huds[Name(p)] and MineStars.Offhand.Huds[Name(p)] ~= {} and MineStars.Offhand.Huds[Name(p)].ItemCount then
			if item then
				if item:get_count() > 0 then
					--Wear
					local wear = item:get_wear()
					if wear > 0 then
						local wear_percent = (65535 - wear) / 65535
						local color = {255, 255, 255}
						local final_wear = wear / 65535
						local wear_i = math.min(math.floor(final_wear * 600), 511)
						wear_i = math.min(wear_i + 10, 511)
						if wear_i <= 255 then
							color = {wear_i, 255, 0}
						else
							color = {255, 511 - wear_i, 0}
						end
						p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBar, "text", "offhand_wear_bar.png^[colorize:#"..rgb_to_hex(color[1], color[2], color[3]))
						p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBar, "scale", {x = 40 * wear_percent, y = 3})
						p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBar, "offset", {x = -320 - (20 - 10 / 2), y = -13})
						p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBarBg, "text", "offhand_wear_bar.png")
					else
						p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBarBg, "text", "blank.png")
						p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBar, "text", "blank.png")
					end
					
					--Textures
					--Default
					p:hud_change(MineStars.Offhand.Huds[Name(p)].Slot, "text", "offhand_slot.png")
					p:hud_change(MineStars.Offhand.Huds[Name(p)].Item, "text", ReturnIcon(item:get_definition()))
					if item:get_count() > 1 then
						p:hud_change(MineStars.Offhand.Huds[Name(p)].ItemCount, "text", item:get_count())
					end
				else
					p:hud_change(MineStars.Offhand.Huds[Name(p)].ItemCount, "text", "")
					p:hud_change(MineStars.Offhand.Huds[Name(p)].Slot, "text", "blank.png")
					p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBarBg, "text", "blank.png")
					p:hud_change(MineStars.Offhand.Huds[Name(p)].Item, "text", "blank.png")
					p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBar, "text", "blank.png")
				end
			else
				p:hud_change(MineStars.Offhand.Huds[Name(p)].ItemCount, "text", "")
				p:hud_change(MineStars.Offhand.Huds[Name(p)].Slot, "text", "blank.png")
				p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBarBg, "text", "blank.png")
				p:hud_change(MineStars.Offhand.Huds[Name(p)].Item, "text", "blank.png")
				p:hud_change(MineStars.Offhand.Huds[Name(p)].WearBar, "text", "blank.png")
			end
		end
	end, p, ITM)
end

--Wield Item
--Fuck you, Rubenwardy, Lonewolf, dakoda, xXNicoXx, and other idiots.

MineStars.Offhand.OffhandObjs = {}

core.register_entity("engine:wield_item_offhand", {
	initial_properties = {
		hp_max = 1,
		hp = 1,
		visual = "wielditem",
		textures = {"engine:empty_hand"},
		visual_size = vector.new(0.21,0.21,0.21),
		collisionbox = {
			0.1,
			0.1,
			0.1,
			-0.1,
			-0.1,
			-0.1,
		},
		pointable = false,
		physical = false,
		static_save = false,
	},
	item = "",
	--on_step = function(self, dtime)
	--	print("acT"..dtime)
	--end
})

core.register_item("engine:empty_hand", {
	type = "none",
	wield_image = "blank.png",
})

local function return_name_or_empty(name)
	--print(dump(name))
	if name == "" then
		return "engine:empty_hand"
	else
		return name
	end
end

function MineStars.Offhand.SetPhysicalHand(player)
	if not MineStars.Offhand.OffhandObjs[Name(player)] then
		local pos = player:get_pos()
		local inv = return_inv_or_create(player)
		if pos then
			local obj = core.add_entity(pos, "engine:wield_item_offhand")
			if obj then
				--obj:set_attach(player, "", vector.new(-3.3,7,2), vector.new(90,0,0), true) --Fail safe for 0.4
				obj:set_attach(player, "Arm_Left", vector.new(0,-3,0), vector.new(0, 180, 0))
				obj:set_properties({
					textures = {return_name_or_empty(inv:get_stack("offhand", 1):get_name())}
				})
				MineStars.Offhand.OffhandObjs[Name(player)] = obj
				local ent = obj:get_luaentity()
				if ent then
					ent.item = return_name_or_empty(inv:get_stack("offhand", 1):get_name())
				end
			end
		end
	else
		MineStars.Offhand.UpdatePhysicalHand(player) -- update
	end
end

function MineStars.Offhand.UpdatePhysicalHand(player, stack)
	if MineStars.Offhand.OffhandObjs[Name(player)] then
		--return_name_or_empty(inv:get_stack("offhand", 1):get_name() --return_name_or_empty()
		local item = stack or return_inv_or_create(player):get_stack("offhand", 1)
		if item then
			local obj = MineStars.Offhand.OffhandObjs[Name(player)]-- copy
			if obj then
				local ent = obj:get_luaentity()
				obj:set_properties({
					textures = {return_name_or_empty(item:get_name())}
				})
				if ent then
					ent.item = return_name_or_empty(item:get_name())
				end
			end
		end
	end
end

core.register_on_leaveplayer(function(player)
	if MineStars.Offhand.OffhandObjs[Name(player)] then
		MineStars.Offhand.OffhandObjs[Name(player)]:remove()
		MineStars.Offhand.OffhandObjs[Name(player)] = nil
	end
end)

core.register_on_dieplayer(function(player)
	MineStars.UpdateHud(player, ItemStack("")) --ItemStack("") clears all
end)

















