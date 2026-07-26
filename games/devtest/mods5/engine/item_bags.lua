--[[
	MineStars Items Bag
--]]

local strg = MineStars.Storage;
local playingIndex = {};

local function helper____(p, inv, id)
	core.after(0.1, function(p)
		--Player disconnected?
		if p:get_pos() == nil then core.log("warning", "Player disconnected IN FUNCTION"); return end
		--Get stack
		local st_ = p:get_wielded_item();
		--Get inventory list & transform to string
		local subinventory_Serialized = {};
		for _, stack in pairs(inv:get_list("main")) do
			subinventory_Serialized[_] = stack:to_string();
		end
		local meta = st_:get_meta();
		local srz = core.serialize(subinventory_Serialized)
		meta:set_string("inventory_", srz);
		p:set_wielded_item(st_)
		core.log("info", "Saved inventory for "..Name(p)..", ID: "..id.." "..srz)
	end, p)
end

--lua_api.txt => 6227
local function inventory_engine(p, itemstack, mode)
	if mode == "query" then
		local meta = itemstack:get_meta();
		local inventory_str = meta:get_string("inventory_");
		local nameforinv = Name(p).."_detacheditembag_"..math.random(1, (2^16)-1);
		playingIndex[Name(p)] = nameforinv; --debug
		local inv = core.create_detached_inventory(
			nameforinv, --name
			{
				on_put = function(inv, lt, idx, stk, player)
					if player:get_wielded_item():get_name() == "engine:bag" then
						helper____(player, inv, playingIndex[Name(player)]);
					end
				end,
				on_take = function(inv, lt, idx, stk, player)
					if player:get_wielded_item():get_name() == "engine:bag" then
						helper____(player, inv, playingIndex[Name(player)]);
					end
				end,
				on_move = function(inv, from_list, from_index, to_list, to_index, count, player)
					if player:get_wielded_item():get_name() == "engine:bag" then
						helper____(player, inv, playingIndex[Name(player)]);
					end
				end,
			}
		);
		--print("a");
		inv:set_size("main", 16)
		--print("b");
		local list_ = {};
		--print(dump(inventory_str).." CLARAMENTE HIJUEPUTA");
		for _, ITM in pairs(core.deserialize(inventory_str) or {}) do
		--	print("d");
		--	print(ITM)
			list_[_] = ItemStack(ITM);
		end
		--print("e");
		inv:set_list("main", list_);
		--print("f");
		return nameforinv;
	elseif mode == "quit" then --on quit inventory
		core.remove_detached_inventory(playingIndex[Name(p)]);
		return nil
	end
end

local function get_formspec(id)
	return "size[8.01,7.05]" ..
	"list[detached:"..id..";main;0.02,0.02;8.0,1.0;0]" ..
	"list[detached:"..id..";main;0.02,1.11;8.0,1.0;8]" ..
	"list[current_player;main;0.02,2.97;8.0,1.0;0]" ..
	"list[current_player;main;0.02,4.05;8.0,1.0;8]" ..
	"list[current_player;main;0.02,5.14;8.0,1.0;16]" ..
	"list[current_player;main;0.02,6.22;8.0,1.0;24]"
end

local function use(itm, player)
	local id = inventory_engine(player, itm, "query");
	local formspec = get_formspec(id);
	core.show_formspec(player:get_player_name(), "bag__", formspec);
end

core.register_on_player_receive_fields(function(player, frn, f)
	if frn == "bag__" and f.quit then
		inventory_engine(player, nil, "quit");
	end
end)

core.register_tool("engine:bag", {
	description = MineStars.Translator("Bag"),
	inventory_image = "eng_bag.png",
	on_use = use,
	on_secondary_use = use,
	_ID = "", -- value intended
})

core.register_on_leaveplayer(function(player)
	if playingIndex[Name(player)] then
		inventory_engine(player, nil, "quit")
	end
end)





















