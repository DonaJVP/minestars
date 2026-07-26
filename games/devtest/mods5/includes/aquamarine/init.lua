-- Register the new ore
minetest.register_ore({
    ore_type = "scatter",
    ore = "aquamarine:aquamarine_ore",
    wherein = "default:stone",
    clust_scarcity = 8 * 8 * 8,
    clust_num_ores = 7,
    clust_size = 3,
    y_min = -31000,
    y_max = -64,
})

-- Register the ore node
minetest.register_node("aquamarine:aquamarine_ore", {
    description = "Aquamarine Ore",
    tiles = {"aquamarine_aquamarine_ore.png"},
    groups = {cracky = 3, ore = 1},
    drop = "aquamarine:aquamarine_lump",
})

-- Register the block of the ore's product
minetest.register_node("aquamarine:aquamarine_block", {
    description = "Aquamarine Block",
    tiles = {"aquamarine_aquamarine_block.png"},
    is_ground_content = false,
    groups = {cracky = 2, level = 2},
})

-- Register the lump that drops from the ore
minetest.register_craftitem("aquamarine:aquamarine_lump", {
    description = "Aquamarine Lump",
    inventory_image = "aquamarine_aquamarine_lump.png",
})

-- Register the ingot
minetest.register_craftitem("aquamarine:aquamarine_ingot", {
    description = "Aquamarine Ingot",
    inventory_image = "aquamarine_aquamarine_ingot.png",
})

-- Add crafting recipe to turn lumps into ingots
minetest.register_craft({
    type = "cooking",
    output = "aquamarine:aquamarine_ingot",
    recipe = "aquamarine:aquamarine_lump",
    cooktime = 4,  -- time in seconds
})

-- Add crafting recipe to turn ingots into blocks
minetest.register_craft({
    output = "aquamarine:aquamarine_block",
    recipe = {
        {"aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot"},
        {"aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot"},
        {"aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot"},
    }
})

-- Register Aquamarine Pickaxe
minetest.register_tool("aquamarine:aquamarine_pickaxe", {
    description = "Aquamarine Pickaxe",
    inventory_image = "aquamarine_aquamarine_pickaxe.png",
    tool_capabilities = {
        full_punch_interval = 1.0,
        max_drop_level = 3,
        groupcaps = {
            cracky = {
                times = {[1] = 2.5, [2] = 1.5, [3] = 1.0},
                uses = 30,
                maxlevel = 3,
            },
        },
        damage_groups = {fleshy = 5},
    },
})

-- Register Aquamarine Shovel
minetest.register_tool("aquamarine:aquamarine_shovel", {
    description = "Aquamarine Shovel",
    inventory_image = "aquamarine_aquamarine_shovel.png",
    wield_image = "aquamarine_aquamarine_shovel.png^[transformR90",
    tool_capabilities = {
        full_punch_interval = 1.1,
        max_drop_level = 1,
        groupcaps = {
            crumbly = {
                times = {[1] = 1.5, [2] = 0.8, [3] = 0.5},
                uses = 50,
                maxlevel = 3,
            },
        },
        damage_groups = {fleshy = 4},
    },
})

-- Register Aquamarine Sword
minetest.register_tool("aquamarine:aquamarine_sword", {
    description = "Aquamarine Sword",
    inventory_image = "aquamarine_aquamarine_sword.png",
    tool_capabilities = {
        full_punch_interval = 0.9,
        max_drop_level = 1,
        groupcaps = {
            snappy = {
                times = {[1] = 1.6, [2] = 0.8, [3] = 0.4},
                uses = 35,
                maxlevel = 3,
            },
        },
        damage_groups = {fleshy = 7},
    },
})

-- Add the crafting recipes for the tools
minetest.register_craft({
    output = "aquamarine:aquamarine_pickaxe",
    recipe = {
        {"aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot"},
        {"", "default:stick", ""},
        {"", "default:stick", ""}
    }
})

-- Register Aquamarine Axe
minetest.register_tool("aquamarine:aquamarine_axe", {
    description = "Aquamarine Axe",
    inventory_image = "aquamarine_aquamarine_axe.png",
    tool_capabilities = {
        full_punch_interval = 1.0,
        max_drop_level = 1,
        groupcaps = {
            choppy = {
                times = {[1] = 2.00, [2] = 1.00, [3] = 0.50},
                uses = 30,
                maxlevel = 2
            },
        },
        damage_groups = {fleshy = 4},
    },
})

-- Register Aquamarine Axe Crafting Recipe
minetest.register_craft({
    output = "aquamarine:aquamarine_axe",
    recipe = {
        {"aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot", ""},
        {"aquamarine:aquamarine_ingot", "default:stick", ""},
        {"", "default:stick", ""}
    }
})


minetest.register_craft({
    output = "aquamarine:aquamarine_sword",
    recipe = {
        {"", "aquamarine:aquamarine_ingot", ""},
        {"", "aquamarine:aquamarine_ingot", ""},
        {"", "default:stick", ""}
    }
})

minetest.register_craft({
    output = "aquamarine:aquamarine_axe",
    recipe = {
        {"aquamarine:aquamarine_ingot", "aquamarine:aquamarine_ingot", ""},
        {"aquamarine:aquamarine_ingot", "default:stick", ""},
        {"", "default:stick", ""}
    }
})

minetest.register_craft({
    output = "aquamarine:aquamarine_shovel",
    recipe = {
        {"", "aquamarine:aquamarine_ingot", ""},
        {"", "default:stick", ""},
        {"", "default:stick", ""}
    }
})
