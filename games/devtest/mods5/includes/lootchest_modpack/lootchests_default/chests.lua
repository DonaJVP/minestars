local S = minetest.get_translator(minetest.get_current_modname())

lootchests.register_lootchest({
    name = "lootchests_default:ocean_chest",
    description = S("Loot Chest"),
    spawn_in = {"default:sand", "default:desert_sand", "default:silver_sand"},
    spawn_on = {"default:stone",},
    sounds = default.node_sound_wood_defaults(),
    groups = {choppy = 2, oddly_breakable_by_hand = 2},
    ymax = 5,
    ymin = -512,
    spawn_in_rarity = 40,
    spawn_on_rarity = 2500,
    slot_spawn_chance = 75,
    slots = 32,
})

lootchests.register_lootchest({
    name = "lootchests_default:basket",
    description = S("Basket"),
    tiles = {
        "lootchests_default_basket_top.png",
        "lootchests_default_basket_top.png",
        "lootchests_default_basket_side.png",
    },
    spawn_on = {"default:stone"},
    sounds = default.node_sound_wood_defaults(),
    groups = {snappy = 2, oddly_breakable_by_hand = 2},
    ymax = 0,
    ymin = -256,
    spawn_on_rarity = 700,
    slot_spawn_chance = 35,
    slots = 24,
})

lootchests.register_lootchest({
    name = "lootchests_default:urn",
    description = S("Urn"),
    tiles = {
        "lootchests_default_urn_top.png",
        "lootchests_default_urn_top.png",
        "lootchests_default_urn_side.png",
    },
    spawn_on = {"default:stone"},
    sounds = default.node_sound_stone_defaults(),
    groups = {cracky = 2, oddly_breakable_by_hand = 2},
    ymax = 0,
    ymin = -256,
    spawn_on_rarity = 900,
    slot_spawn_chance = 50,
    slots = 24,
})

lootchests.register_lootchest({
    name = "lootchests_default:barrel",
    description = S("Barrel"),
    tiles = {
        "lootchests_default_barrel_top.png",
        "lootchests_default_barrel_top.png",
        "lootchests_default_barrel_side.png",
    },
    spawn_in = {"default:sand", "default:desert_sand", "default:silver_sand"},
    sounds = default.node_sound_wood_defaults(),
    groups = {choppy = 2, oddly_breakable_by_hand = 2},
    ymax = 5,
    ymin = -32,
    spawn_in_rarity = 35,
    slot_spawn_chance = 50,
    slots = 24,
})

lootchests.register_lootchest({
    name = "lootchests_default:stone_chest",
    description = S("Ancient Chest"),
    tiles = {
        "lootchests_default_stone_chest_top.png",
        "lootchests_default_stone_chest_top.png",
        "lootchests_default_stone_chest_side.png",
        "lootchests_default_stone_chest_side.png",
        "lootchests_default_stone_chest_front.png",
    },
    spawn_in = {"default:stone"},
    spawn_on = {"default:stone"},
    sounds = default.node_sound_stone_defaults(),
    groups = {cracky = 2},
    ymax = -128,
    ymin = -4096,
    spawn_in_rarity = 75,
    spawn_on_rarity = 7500,
    slot_spawn_chance = 75,
    slots = 32,
})
