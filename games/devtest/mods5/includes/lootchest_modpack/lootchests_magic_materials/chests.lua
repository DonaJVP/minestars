local S = minetest.get_translator(minetest.get_current_modname())

lootchests.register_lootchest({
    name = "lootchests_magic_materials:rune_chest",
    description = S("Rune Chest"),
    tiles = {
        "lootchests_magic_materials_rune_chest_top.png",
        "lootchests_magic_materials_rune_chest_top.png",
        "lootchests_magic_materials_rune_chest_side.png",
        "lootchests_magic_materials_rune_chest_side.png",
        "lootchests_magic_materials_rune_chest_front.png",
    },
    spawn_in = "default:stone",
    spawn_on = "default:stone",
    sounds = default.node_sound_stone_defaults(),
    groups = {cracky = 2, oddly_breakable_by_hand = 2},
    ymax = -64,
    ymin = -4096,
    spawn_in_rarity = 70,
    spawn_on_rarity = 5000,
    slot_spawn_chance = 75,
    slots = 24,
})

lootchests.register_lootchest({
    name = "lootchests_magic_materials:rune_urn",
    description = S("Rune Urn"),
    tiles = {
        "lootchests_magic_materials_rune_urn_top.png",
        "lootchests_magic_materials_rune_urn_top.png",
        "lootchests_magic_materials_rune_urn_side.png",
    },
    spawn_on = "default:stone",
    sounds = default.node_sound_stone_defaults(),
    groups = {cracky = 2, oddly_breakable_by_hand = 2},
    ymax = -64,
    ymin = -4096,
    spawn_on_rarity = 1800,
    slot_spawn_chance = 50,
    slots = 24,
})
