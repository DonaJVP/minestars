--MINESTARS MOD "COMPASS"

--skipped....
do
	return
end

Compass = {
	storage = MineStars.Storage,
	Configurations = {
		-- TEST CONFIG
		Test = {
			description = "Test tool",
			inventory_image = "default_wood.png",
			on_exec = function(player)
				print("GOT")
				return true -- should delete all items when executed?
			end
		},
		Test2 = {
			description = "Test tool TWO",
			inventory_image = "default_stone.png",
			on_exec = function(player)
				return true -- should delete all items when executed?
			end
		}
	},
	Tool = "compass_",
	Inv = {
		--[[
		[1] = ItemStack("default:apple")
		--]]
	}
}

function Compass.IsInSpawn(player)
	return true -- To be overrinden by other modules
end

--Initialize the positions

local storage = Compass.storage or nil

do
	if storage or Compass.storage then
		local data = core.deserialize(storage:get_string("saved_configurations")) or {}
		
	end
end

--Register itemstack with specific options
core.register_on_mods_loaded(function()
	for ELEMENT, DEF in pairs(Compass.Configurations) do
		core.log("action", "Adding a Item onto list for compass `"..DEF.description.."`")
		local stack = ItemStack(Compass.Tool)
		local meta = stack:get_meta()
		meta:set_string("inventory_image", DEF.inventory_image)
		meta:set_string("description", DEF.description)
		meta:set_string("COMPASS", "true")
		meta:set_string("NAME", ELEMENT)
		table.insert(Compass.Inv, stack)
	end
	--Do like a inventory of 32 slots
	for i = 1, 32 do
		if not Compass.Inv[i] then
			core.log("action", "Inserting slot to Compass inventory '"..i.."'")
			Compass.Inv[i] = ItemStack("")
		end
	end
end)

function FUNC(itemstack, player)
	local meta = itemstack:get_meta()
	if meta then
		if meta:get_string("COMPASS") == "true" then
			local typ = meta:get_string("NAME")
			if Compass.Configurations[typ] then
				local exec = Compass.Configurations[typ].on_exec
				local returns = exec(player)
				if returns then
					local last_inv = player:get_inventory():get_list("main_hidden")
					if last_inv then
						player:get_inventory():set_list("main", last_inv)
					end
				end
			end
		end
	end
end

core.register_tool(":compass_", {
	inventory_image = "default_apple.png",
	description = "Compass",
	COMPASS = "true",
	NAME = "",
	on_use = FUNC,
	on_place = FUNC,
	on_secondary_use = FUNC,
})

core.register_on_joinplayer(function(player)
	local inv = player:get_inventory()
	if inv then
		inv:set_size("main_hidden", 32)
		--Check if inventory has already those tools
		local _ = inv:get_list("main")
		if _[1] and _[1]:get_name() == "compass_" then
			return
		end
		if Compass.IsInSpawn(player) then
			if _ then
				inv:set_list("main_hidden", _)
			end
			inv:set_list("main", Compass.Inv)
		end
	end
end)

function Compass.GettingOutOfSpawn(player)
	local inv = player:get_inventory()
	if inv then
		inv:set_list("main", inv:get_list("main_hidden"))
	end
end

--
-- CENTRAL
--

local ms = 0
local limit = 0
core.register_globalstep(function(dt)
	ms = ms + dt
	if ms > 3 then
		ms = 0
		local spawnpos = spw.pos.lobby
		for _, p in pairs(core.get_connected_players()) do
			local arpos = p:get_pos()
			if vector.distance(arpos, spawnpos) > 40 then
				--Delete inventory
				if p:get_inventory():get_list("main")[1]:get_name() == "compass_" then
					p:get_inventory():set_list("main", p:get_inventory():get_list("main_hidden"))
				end
			end
		end
	end
end)













