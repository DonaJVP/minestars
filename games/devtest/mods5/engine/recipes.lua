--Workbench
--[[core.register_craft({
	output = "default:mallet",
	recipe = {
		{"default:steel_ingot", "default:steel_ingot", "default:steel_ingot"},
		{"default:steel_ingot", "default:steel_ingot", "default:steel_ingot"},
		{"group:stick"},
	}
})--]]

local olfunc = core.register_craft
function core.register_craft(craft)
	local f1 = craft.recipe[1] or ""
	local f2 = craft.recipe[2] or ""
	local f3 = craft.recipe[3] or ""
	local f4 = craft.recipe[4] or ""
	local f5 = craft.recipe[5] or ""
	local f6 = craft.recipe[6] or ""
	local f7 = craft.recipe[7] or ""
	local f8 = craft.recipe[8] or ""
	local f9 = craft.recipe[9] or ""
	
	local ff1 = {f1,f2,f3}
	local ff2 = {f4,f5,f6}
	local ff3 = {f7,f8,f9}
	olfunc({
		output = craft.output,
		recipe = {ff1,ff2,ff3}
	})
end

core.register_craft({
	output = "engine:table_of_work",
	recipe = {"default:stone","default:stone","default:stone","default:stone","","default:stone","default:stone","","default:stone"}
})

core.register_craft({
	output = "engine:void_kicker",
	recipe = {"", "", "void:void_ingot", "", "group:stick", "", "group:stick", "", ""}
})

core.register_craft({
	output = "engine:teleport_raw",
	recipe = {"void:void_ingot","void:void_ingot","","void:void_ingot","void:void_ingot","","","",""}
})

core.register_craft({
	output = "engine:nickel_compass",
	recipe = {"nickel:nickel_ingot","nickel:nickel_ingot","nickel:nickel_ingot","nickel:nickel_ingot","cobalt:cobalt_ingot","nickel:nickel_ingot","nickel:nickel_ingot","nickel:nickel_ingot","nickel:nickel_ingot"}
})

core.register_craft({
	output = "engine:elestone_ingot",
	recipe = {"sunstone:sunstone_ingot","elementium:elementium_ingot"}
})

core.register_craft({
	output = "engine:elestone_axe",
	recipe = {"engine:elestone_ingot","engine:elestone_ingot","","engine:elestone_ingot","group:stick","","","group:stick",""}
})

core.register_craft({
	output = "engine:bloodstone_rock",
	recipe = {"bloodstone:bloodstone_ingot", "default:apple"}
})

core.register_craft({
	output = "engine:emerald_staff",
	recipe = {"","","emerald:emerald_ingot","","group:stick","","group:stick","",""}
})

core.register_craft({
	output = "engine:onyx_dagger",
	recipe = {"","onyx:onyx_ingot","","group:stick","","","","",""}
})

core.register_craft({
	output = "engine:faction_chest",
	recipe = {"default:wood","default:wood","default:wood","default:wood","nickel:nickel_ingot","default:wood","default:wood","default:wood","default:wood"}
})

core.register_craft({
	output = "engine:factions_territory_pole",
	recipe = {"default:steel_block","default:steel_block","default:steel_block","default:wood","nickel:nickel_ingot","default:wood","default:steel_ingot","default:steel_ingot","default:steel_ingot"}
})

core.register_craft({
	output = "engine:atm",
	recipe = {"default:steel_ingot","default:steel_ingot","default:steel_ingot","default:steel_ingot","engine:paper_with_value_of_1","default:steel_ingot","default:steel_ingot","default:steel_ingot","default:steel_ingot"}
})

local w = "default:wood"
local m = "engine:paper_with_value_of_1"

core.register_craft({
	output = "engine:shop",
	recipe = {w,w,w,w,m,w,w,w,w}
})

core.register_craft({
	output = "engine:areas_security_1",
	recipe = {"default:steel_ingot","default:steel_ingot","default:steel_ingot","default:steel_ingot","basic_materials:padlock","default:steel_ingot","default:steel_ingot","default:steel_ingot","default:steel_ingot"}
})

core.register_craft({
	output = "engine:areas_security_2",
	recipe = {"engine:areas_security_1","basic_materials:padlock","","","","","","",""}
})

core.register_craft({
	output = "elementium:elementium_ingot",
	recipe = {"engine:heaven_stone","sunstone:sunstone_ingot","moonstone:moonstone_ingot","bloodstone:bloodstone_ingot","","","","",""},
})

core.register_craft = olfunc



