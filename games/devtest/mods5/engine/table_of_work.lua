--MineStars Table Of Work

local function rp(p)
	return p.x..","..p.y..","..p.z
end

MineStars.Workbench = {
	returnformspec = function(itemname, able_to_change_name, reasonofchange, pos)
		local form = "size[8,8]".."box[-0.3,-0.3;8.35,0.35;#009200]".."label[0,-0.38;Workbench]".."list[current_player;main;0,3.4;8,4;]".."label[2.65,1.6;Item]".."list[nodemeta:"..rp(pos)..";mallet_item_place;2.5,2;1,1;]".."label[0.3,1.6;Mallet]".."list[nodemeta:"..rp(pos)..";mallet_place;0.2,2;1,1;]".."label[5.8,1.6;Result]".."list[nodemeta:"..rp(pos)..";mallet_result;5.75,2;1,1;]"
		local addon = "style[setname;bgcolor=red;textcolor=white]"
		if able_to_change_name then
			addon = "style[setname;bgcolor=green;textcolor=white]"
		end
		form = form..addon.."image[4.1,2;1,1;gui_furnace_arrow_bg.png^[transformR270]".."image[1.3,2;1,1;plus.png]".."field[0.3,0.7;6,1.2;nameofitem;Name for item "..core.formspec_escape(((reasonofchange or "") ~= "" and "["..(reasonofchange or "").."]") or "")..";"..itemname.."]".."image_button[5.8,0.58;2.25,0.84;blank.png;setname;Set name]".."image_button_exit[0,7.5;8.04,0.9;blank.png;quit;Quit | Exit]"
		return form
	end,
	Cache = {},
	CachePos = {},
	Registered = {
		["default:stone"] = "default:cobble", --example
	},
}

--Mallet
--[[
core.register_tool(":default:mallet", {
	description = MineStars.Translator("Mallet"),
	inventory_image = "default_mallet.png",
	tool_capabilities = {
		full_punch_interval = 2,
		max_drop_level=3,
		groupcaps={
			cracky = {times={[1]=2.5, [2]=1.4, [3]=0.30}, uses=20, maxlevel=3},
		},
		damage_groups = {fleshy=13},
	},
	sound = {breaks = "default_tool_breaks"},
	groups = {pickaxe = 1},
	on_use = function(item, player, pointed_thing)
		if pointed_thing.type == "node" then
			local pos = pointed_thing.under
			if not core.is_protected(pos, Name(player)) then
				local node = core.get_node(pos)
				if MineStars.Workbench.Registered[node.name] then
					core.swap_node(pos, {name = MineStars.Workbench.Registered[node.name]})
					item:set_wear(item:get_wear() + 500)
					return item
				end
			end
		end
	end,
})
--]]
--Table

core.register_node("engine:table_of_work", {
	description = MineStars.Translator("Workbench"),
	drawtype = "nodebox",
	paramtype = "light",
	paramtype2 = "facedir",
	sunlight_propagates = true,
	tiles = { "default_furnace_top.png" },
	groups = groups,
	node_box = {
		type = "fixed",
		fixed = {
			{ -0.4, -0.5, -0.4, -0.3, 0.4, -0.3 }, -- foot 1
			{ 0.3, -0.5, -0.4, 0.4, 0.4, -0.3 }, -- foot 2
			{ -0.4, -0.5, 0.3, -0.3, 0.4, 0.4 }, -- foot 3
			{ 0.3, -0.5, 0.3, 0.4, 0.4, 0.4 }, -- foot 4
			{ -0.5, 0.4, -0.5, 0.5, 0.5, 0.5 } -- table top
		},
	},
	after_place_node = function(pos, placer)
		local meta = core.get_meta(pos)
		if meta then
			meta:set_string("owner", Name(placer))
			meta:set_string("infotext", MineStars.Translator("Workbench (Owned by @1)", Name(placer)))
		end
	end,
	on_construct = function(pos)
		local meta = core.get_meta(pos)
		local inv = meta:get_inventory()
		inv:set_size("mallet_item_place", 1)
		inv:set_size("mallet_place", 1)
		inv:set_size("mallet_result", 1)
	end,
	on_rightclick = function(pos, _, player, itemstack)
		if Name(player) then
			local can_interact = not core.is_protected(pos, Name(player))
			local Xp = MineStars.XP.GetXP(player)
			local owner = core.get_meta(pos):get_string("owner")
			if owner == Name(player) or can_interact then
				core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec("No item", (Xp >= 50), nil, pos))
				MineStars.Workbench.Cache[Name(player)] = false --itemcache name
				MineStars.Workbench.CachePos[Name(player)] = pos
			end
		end
	end,
	--metaref
	allow_metadata_inventory_put = function(pos, listname, index, stack, player)
		local meta = core.get_meta(pos)
		if (meta:get_string("owner") == Name(player)) or not core.is_protected(pos, Name(player)) then
			if listname == "mallet_place" then
				if stack and stack:get_name() == "default:mallet" then
					if Name(player) then
						if vector.distance(player:get_pos(), pos) < 6 then
							local Xp = MineStars.XP.GetXP(player)
							MineStars.Workbench.Cache[Name(player)] = stack:get_short_description()
							core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec(stack:get_short_description(), (Xp > 50), nil, pos))
							--malletresult
							local itemname = MineStars.Workbench.GetMalletResult(meta:get_inventory():get_stack("mallet_item_place", 1))
							if itemname then
								local stack__ = meta:get_inventory():get_stack("mallet_item_place", 1)
								stack__:set_name(itemname)
								--stack__:set_count(stack__:get_count() + stack:get_count())
								meta:get_inventory():set_stack("mallet_result", 1, stack__)
							end
						end
					end
					return stack:get_count()
				else
					return 0
				end
			else
				if listname == "mallet_item_place" then
					if Name(player) then
						if vector.distance(player:get_pos(), pos) < 6 then
							local Xp = MineStars.XP.GetXP(player)
							MineStars.Workbench.Cache[Name(player)] = stack:get_short_description()
							core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec(stack:get_short_description(), (Xp > 50), nil, pos))
							--malletresult
							local itemname = MineStars.Workbench.GetMalletResult(stack)
							if itemname then
								local stack__ = stack
								stack__:set_name(itemname)
								stack__:set_count(stack__:get_count() + meta:get_inventory():get_stack("mallet_item_place", 1):get_count())
								meta:get_inventory():set_stack("mallet_result", 1, stack__)
							end
						end
					end
					return stack:get_count()
				else
					return 0
				end
			end
		else
			return 0
		end
		return stack:get_count()
	end,
	allow_metadata_inventory_take = function(pos, listname, index, stack, player)
		local meta = core.get_meta(pos)
		if (meta:get_string("owner") == Name(player)) or not core.is_protected(pos, Name(player)) then
			if listname == "mallet_result" then
				local malletstack = meta:get_inventory():get_stack("mallet_place", 1)
				if malletstack:get_count() > 0 then
					MineStars.Workbench.ExhaustMallet(pos, stack:get_count())
					--print(stack:get_count())
					local stack_ = meta:get_inventory():get_stack("mallet_item_place", 1)
					stack_:set_count(stack_:get_count() - stack:get_count())
					meta:get_inventory():set_stack("mallet_item_place", 1, stack_)
				end
			elseif listname == "mallet_place" then
				meta:get_inventory():set_stack("mallet_result", 1, nil)
			elseif listname == "mallet_item_place" then
				--if stack:get_count() > 0 then
					local name = MineStars.Workbench.GetMalletResult(stack)
					if name then
						local count = meta:get_inventory():get_stack("mallet_item_place", 1):get_count() - stack:get_count()
						local itm = meta:get_inventory():get_stack("mallet_item_place", 1)
						itm:set_count(count)
						--print("NABDLIEUSBFUIKSJBEOIBFL")
						itm:set_name(name)
						meta:get_inventory():set_stack("mallet_result", 1, itm)
					end
				--end
			end
			return (meta:get_inventory():get_stack("mallet_place", 1):get_count() > 0 and stack:get_count()) or 0
		else
			return 0
		end
	end,
	allow_metadata_inventory_move = function(pos, from_list, from_index, to_list, to_index, count, player)
		if to_list == "mallet_result" then
			return 0
		end
		local inv = core.get_meta(pos):get_inventory()
		local stack = inv:get_stack(from_list, from_index)
		if to_list == "mallet_place" and not ((stack:get_count() > 0) and (stack:get_name() == "default:mallet")) then
			return 0
		end
		return count
	end,
})

function MineStars.Workbench.ExhaustMallet(pos, count)
	local inv = core.get_meta(pos):get_inventory()
	local wear = count * 5000 -- 0 ~ 65535
	local has_to_delete_item = false
	if wear > 65535 then
		has_to_delete_item = true
	end
	if not has_to_delete_item then
		local stack = inv:get_stack("mallet_place", 1)
		stack:set_wear(wear)
		inv:set_stack("mallet_place", 1, stack)
	else
		local stack = ItemStack("")
		inv:set_stack("mallet_place", 1, stack)
	end

end

function MineStars.Workbench.RegisterMalletResult(def)
	local name = def.input
	local out = def.output
	MineStars.Workbench.Registered[name] = out
end

function MineStars.Workbench.GetMalletResult(itemstack)
	local name = itemstack:get_name()
	--if MineStars.Workbench.Registered[name] then
		return MineStars.Workbench.Registered[name]
	--end
end

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "table_of_work" then
		local Xp = MineStars.XP.GetXP(player)
		local pos = MineStars.Workbench.CachePos[Name(player)]
		if pos then
			local inv = core.get_meta(pos):get_inventory()
			local malletstack = inv:get_stack("mallet_place", 1)
			local item_stack = inv:get_stack("mallet_item_place", 1)
			local thereismallet = malletstack and malletstack:get_name() and malletstack:get_name() == "default:mallet"
			local thereisitem = item_stack and item_stack:get_count() > 0
			--check smth
			if not thereisitem then
				inv:set_stack("mallet_result", 1, nil)
			end
			if fields.nameofitem then
				if fields.nameofitem ~= "" then
					if fields.nameofitem:len() <= 20 then
						MineStars.Workbench.Cache[Name(player)] = fields.nameofitem
					else
						core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec(fields.nameofitem, (Xp > 50), MineStars.Translator("Name too big (Limit is 20chars)"), pos))
					end
				end
			end
			if fields.setname then
				if not MineStars.Workbench.Cache[Name(player)] then
					core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec("No item", (Xp > 50), MineStars.Translator("Specify a name!"), pos))
				else
					if Xp > 50 then
						if thereismallet then
							if thereisitem then
								core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec(MineStars.Workbench.Cache[Name(player)], (Xp > 50), nil, pos))
								local item_meta = item_stack:get_meta()
								item_meta:set_string("description", MineStars.Workbench.Cache[Name(player)])
								inv:set_stack("mallet_result", 1, item_stack)
								local stack2 = item_stack
								stack2:set_count(stack2:get_count() - 1)
								inv:set_stack("mallet_item_place", 1, stack2)
							else
								core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec(MineStars.Workbench.Cache[Name(player)], false, MineStars.Translator("Atleast add a item!"), pos))
							end
						else
							core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec(MineStars.Workbench.Cache[Name(player)], false, MineStars.Translator("You need a mallet!"), pos))
						end
					else
						core.show_formspec(Name(player), "table_of_work", MineStars.Workbench.returnformspec(MineStars.Workbench.Cache[Name(player)], false, MineStars.Translator("No enough XP"), pos))
					end
				end
			end
		end
	end
end)








