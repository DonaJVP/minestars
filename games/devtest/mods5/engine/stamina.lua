MineStars.Food = {}

function MineStars.Food.GetLevel(__obj)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local int = meta:get_int("food_level")
			if int == 65535 then
				return 20
			end
			return int
		end
	end
end
function MineStars.Food.SumLevel(__obj, i)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local int = meta:get_int("food_level")
			if int == 65535 then
				int = 20
			end
			local int = ReturnNonUpperLimit(int + i, 20)
			meta:set_int("food_level", int)
			MineStars.DefaultHuds.UpdateHungerLevel(player, int)
			return int
		end
	end
end
function MineStars.Food.RmvLevel(__obj, i)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local int = meta:get_int("food_level")
			if int == 65535 then
				int = 20
			end
			local int = AlwaysReturnPositiveInteger(int - i)
			meta:set_int("food_level", int)
			MineStars.DefaultHuds.UpdateHungerLevel(player, int)
			return int
		end
	end
end
--[[
core.register_on_mods_loaded(function()
	
end)
--]]

local function show_eat_particles(player, itemname)
	-- particle effect when eating
	itemname=itemname:get_name()
	local pos = player:get_pos()
	pos.y = pos.y + (player:get_properties().eye_height * .923) -- assume mouth is slightly below eye_height
	local dir = player:get_look_dir()
	local def = minetest.registered_items[itemname]
	local texture = def.inventory_image or def.wield_image
	local particle_def = {
		amount = 5,
		time = 0.1,
		minpos = pos,
		maxpos = pos,
		minvel = {x = dir.x - 1, y = dir.y, z = dir.z - 1},
		maxvel = {x = dir.x + 1, y = dir.y, z = dir.z + 1},
		minacc = {x = 0, y = -5, z = 0},
		maxacc = {x = 0, y = -9, z = 0},
		minexptime = 1,
		maxexptime = 1,
		minsize = 1,
		maxsize = 2,
	}
	if texture and texture ~= "" then
		particle_def.texture = texture
	elseif def.type == "node" then
		particle_def.node = {name = itemname, param2 = 0}
	else
		particle_def.texture = "blank.png"
	end
	minetest.add_particlespawner(particle_def)
end

core.do_item_eat = function(hp_change, replace_with_item, itemstack, player, pointed_thing)
	if Name(player) then
		for _, callback in ipairs(minetest.registered_on_item_eats) do
			local result = callback(hp_change, replace_with_item, itemstack, player, pointed_thing)
			if result then
				return result
			end
		end
		if MineStars.Food.GetLevel(player) < 20 then
			if hp_change > 0 then
				MineStars.Food.SumLevel(player, hp_change)
				core.sound_play("stamina_eat", {pos = player:get_pos(), gain = 0.7}, true)
				show_eat_particles(player, itemstack)
				itemstack:take_item()
				replace_with_item = ItemStack(replace_with_item)
				if not replace_with_item:is_empty() then
					local inv = player:get_inventory()
					replace_with_item = inv:add_item("main", replace_with_item)
					if not replace_with_item:is_empty() then
						local pos = player:get_pos()
						pos.y = math.floor(pos.y - 1.0)
						minetest.add_item(pos, replace_with_item)
					end
				end
			else
				player:set_hp(hp_change)
				core.sound_play("stamina_eat", {pos = player:get_pos(), gain = 0.7}, true)
				show_eat_particles(player, itemstack)
				itemstack:take_item()
				replace_with_item = ItemStack(replace_with_item)
				if not replace_with_item:is_empty() then
					local inv = player:get_inventory()
					replace_with_item = inv:add_item("main", replace_with_item)
					if not replace_with_item:is_empty() then
						local pos = player:get_pos()
						pos.y = math.floor(pos.y - 1.0)
						minetest.add_item(pos, replace_with_item)
					end
				end
			end
			return itemstack
		else
			return itemstack
		end
	end
end
--[[
local to_plus = 2
local steps = 0
local substeps = 0
local daily_check = 0
core.register_globalstep(function(dt)
	steps = steps + dt
	substeps = substeps + dt
	daily_check = daily_check + dt
	if daily_check >= 1 then
		daily_check = 0
		for _, p in pairs(core.get_connected_players()) do
			local lvl = MineStars.Food.GetLevel(p)
			if lvl == 0 or lvl < 0 then
				p:set_hp(p:get_hp()-1)
			end
		end
	end
	if steps >= 3 then
		for _, p in pairs(core.get_connected_players()) do
			--local name = p:get_player_name()
			--if name then
				if (p:get_hp() < 20 and p:get_hp() > 0)and MineStars.Food.GetLevel(p) > 4 then
					local hp = p:get_hp()
					local int = ReturnNonUpperLimit(hp + 2, 20)
					p:set_hp(int)
					MineStars.Food.RmvLevel(p, 1)
				end
			--end
		end
		steps = 0
	end
	if substeps >= 60 then
		for _, p in pairs(core.get_connected_players()) do
			MineStars.Food.RmvLevel(p, math.random(1, 2))
		end
		substeps = 0
	end
end)--]]

core.register_on_respawnplayer(function(player)
	MineStars.Food.SumLevel(player, 20)
end)

core.register_on_newplayer(function(player)
	local meta = player:get_meta()
	if meta then
		meta:set_int("food_level", 65535)
	end
end)

--=> 08/02/25

MineStars.Food.Speed = {};

MineStars.Food.IncrementSpeed = function(player)
	if MineStars.Food.Speed[Name(player)] then return end;
	MineStars.Physics.DefaultPhysics[player:get_player_name()].speed = MineStars.Physics.DefaultPhysics[player:get_player_name()].speed + 0.5;
	MineStars.Physics.DefaultPhysics[player:get_player_name()].jump = MineStars.Physics.DefaultPhysics[player:get_player_name()].jump + 0.3;
	MineStars.Physics.Apply(player);
	MineStars.Food.Speed[Name(player)] = true;
end

MineStars.Food.DecrementSpeed = function(player)
	if not MineStars.Food.Speed[Name(player)] then return end;
	MineStars.Physics.DefaultPhysics[player:get_player_name()].speed = MineStars.Physics.DefaultPhysics[player:get_player_name()].speed - 0.5;
	MineStars.Physics.DefaultPhysics[player:get_player_name()].speed = MineStars.Physics.DefaultPhysics[player:get_player_name()].jump - 0.3;
	MineStars.Physics.Apply(player);
	MineStars.Food.Speed[Name(player)] = false
end





