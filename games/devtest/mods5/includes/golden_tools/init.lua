-- Golden Tools for Minetest
local S = minetest.get_translator("golden_tools")

-- Tools (Pickaxe, Sword, Shovel and Axe)
minetest.register_tool("golden_tools:golden_pickaxe", {
	description = S("Golden Pickaxe"),
	inventory_image = "golden_tools_pickaxe.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=1,
		groupcaps={
			cracky = {times={[1]=1.00, [2]=0.95, [3]=0.20}, uses=9, maxlevel=3},
		},
		damage_groups = {fleshy=4},
	},
	sound = {breaks = "default_tool_breaks"},
	groups = {pickaxe = 1}
})
minetest.register_tool("golden_tools:golden_shovel", {
	description = S("Golden Shovel"),
	inventory_image = "golden_tools_shovel.png",
	wield_image = "golden_tools_shovel.png^[transformR90",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=1,
		groupcaps={
			crumbly = {times={[1]=0.60, [2]=0.25, [3]=0.05}, uses=4, maxlevel=3},
		},
		damage_groups = {fleshy=4},
	},
	sound = {breaks = "default_tool_breaks"},
	groups = {shovel = 1}
})
minetest.register_tool("golden_tools:golden_axe", {
	description = S("Golden Axe"),
	inventory_image = "golden_tools_axe.png",
	tool_capabilities = {
		full_punch_interval = 0.9,
		max_drop_level=1,
		groupcaps={
			choppy={times={[1]=1.20, [2]=0.50, [3]=0.10}, uses=3, maxlevel=3},
		},
		damage_groups = {fleshy=7},
	},
	sound = {breaks = "default_tool_breaks"},
	groups = {axe = 1}
})
minetest.register_tool("golden_tools:golden_sword", {
	description = S("Golden Sword"),
	inventory_image = "golden_tools_sword.png",
	tool_capabilities = {
		full_punch_interval = 0.5,
		max_drop_level=1,
		groupcaps={
			snappy={times={[1]=0.30, [2]=0.20, [3]=0.10}, uses=3, maxlevel=3},
		},
		damage_groups = {fleshy=8},
	},
	sound = {breaks = "default_tool_breaks"},
	groups = {sword = 1}
})
-- CRAFTING
minetest.register_craft({
	output = "golden_tools:golden_pickaxe",
	recipe = {
		{"default:gold_ingot", "default:gold_ingot", "default:gold_ingot"},
		{"", "group:stick", ""},
		{"", "group:stick", ""},
	}
})
minetest.register_craft({
	output = "golden_tools:golden_axe",
	recipe = {
		{"default:gold_ingot", "default:gold_ingot"},
		{"default:gold_ingot", "group:stick"},
		{"", "group:stick"},
	}
})
minetest.register_craft({
	output = "golden_tools:golden_sword",
	recipe = {
		{"default:gold_ingot"},
		{"default:gold_ingot"},
		{"group:stick"},
	}
})
minetest.register_craft({
	output = "golden_tools:golden_shovel",
	recipe = {
		{"default:gold_ingot"},
		{"group:stick"},
		{"group:stick"},
	}
})
