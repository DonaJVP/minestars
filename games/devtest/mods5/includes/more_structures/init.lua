morestructures = {}

--Towers

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_grass", "default:dirt_with_snow", "default:dirt_with_coniferous_litter"},
	biomes = {"grassland", "deciduous_forest", "coniferous_forest", "taiga", "snowy_grassland"},
	sidelen = 100,
	fill_ratio = 0.0000052,
	y_max = 300,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/tower_1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Wells

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_grass", "default:dirt_with_snow", "default:dirt_with_coniferous_litter"},
	biomes = {"grassland"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = 50,
	y_min = 3,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/well_1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -7,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_grass", "default:dirt_with_snow", "default:dirt_with_coniferous_litter"},
	biomes = {"snowy_grassland"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = 50,
	y_min = 3,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/well_2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -7,
	rotation = "random",
})

--Desert Wells

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.000007,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/well_3.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.000007,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/well_4.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

--Campsites
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_grass"},
	biomes = {"grassland", "deciduous_forest"},
	sidelen = 100,
	fill_ratio = 0.00001,
	y_max = 300,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/campsite_1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -2,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_coniferous_litter"},
	biomes = {"coniferous_forest"},
	sidelen = 100,
	fill_ratio = 0.00001,
	y_max = 300,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/campsite_2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -2,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_rainforest_litter"},
	biomes = {"rainforest"},
	sidelen = 100,
	fill_ratio = 0.00001,
	y_max = 300,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/campsite_3.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -2,
	rotation = "random",
})

--Igloos
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_snow", "default:snowblock"},
	biomes = {"taiga", "snowy_grassland", "icesheet"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = 50,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/igloo_1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_snow", "default:snowblock"},
	biomes = {"taiga", "snowy_grassland", "icesheet"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = 50,
	y_min = 7,	
	place_offset_y = -17,
	schematic = minetest.get_modpath("more_structures") .. "/schematics/igloo_2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_snow", "default:snowblock"},
	biomes = {"taiga", "snowy_grassland", "icesheet"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = 50,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/igloo_3.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -17,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_snow", "default:snowblock"},
	biomes = {"taiga", "snowy_grassland", "icesheet"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = 50,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/igloo_4.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -8,
	rotation = "random",
})

--Sand Pillars
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00003,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/sand_pillar1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00004,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/sand_pillar2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Desert Sand Pillars
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00005,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desertsand_pillar1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00003,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desertsand_pillar2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Silver Sand Pillars
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:silver_sand"},
	biomes = {"cold_desert"},
	sidelen = 50,
	fill_ratio = 0.00005,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/silversand_pillar1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:silver_sand"},
	biomes = {"cold_desert"},
	sidelen = 50,
	fill_ratio = 0.00003,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/silversand_pillar2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Obsidian Pillars
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand", "default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.0000008,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/obsidian_pillar1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand", "default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.0000005,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/obsidian_pillar2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Sand Arches
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00004,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/sand_arch1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00002,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/sand_arch2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

--Desert Sand Arches
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00005,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desertsand_arch1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.00002,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desertsand_arch2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

--Silver Sand Arches
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:silver_sand"},
	biomes = {"cold_desert"},
	sidelen = 50,
	fill_ratio = 0.00005,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/silversand_arch1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:silver_sand"},
	biomes = {"cold_desert"},
	sidelen = 50,
	fill_ratio = 0.00002,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/silversand_arch2.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

--Mese Hoards
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone"},
	sidelen = 100,
	fill_ratio = 0.000004,
	y_max = -1024,
	y_min = -31000,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/mese_hoard.mts",
	flags = "place_center_x, place_center_z, force_placement, all_floors",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "simple",
	place_on = {"default:grass"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = 300,
	y_min = 7,	
	decoration = "more_structures:villagehouse_1",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Jungle Temple
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:dirt_with_rainforest_litter", "default:dirt"},
	sidelen = 100,
	fill_ratio = 0.00002,
	y_max = 100,
	y_min = 5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/jungle_temple.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Stonehenge
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:dirt", "default:dirt_with_grass"},
	biomes = {"grassland"},
	sidelen = 100,
	fill_ratio = 0.000009,
	y_max = 100,
	y_min = 2,
	schematic = minetest.get_modpath("more_structures") .. "/schematics/stonehenge_1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = -1,
	rotation = "random",
})

--Graveyard
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:dirt", "default:dirt_with_grass"},
	biomes = {"deciduous_forest"},
	sidelen = 100,
	fill_ratio = 0.000009,
	y_max = 100,
	y_min = 2,
	schematic = minetest.get_modpath("more_structures") .. "/schematics/graveyard_1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Ruined Hut
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_grass"},
	biomes = {"deciduous_forest"},
	sidelen = 100,
	fill_ratio = 0.000006,
	y_max = 150,
	y_min = 2,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/ruined_hut_1.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

--Spike
minetest.register_node("more_structures:spike", {
	description = ("Spike"),
	drawtype = "plantlike",
	paramtype = "light",
	tiles = {"ms_spike.png"},
	inventory_image = "ms_spike.png",
	wield_image = "ms_spike.png",
	walkable = false,
	damage_per_second = 5,
	groups = {choppy = 1},
	selection_box = {
		type = "fixed",
		fixed = {-0.25, -0.5, -0.25, 0.25, 0.375, 0.25},
	},
	sounds = default.node_sound_wood_defaults(),
})

minetest.register_craft({
	output = "more_structures:spike 3",
	recipe = {
		{"", "", ""},
		{"default:steel_ingot", "default:steel_ingot", "default:steel_ingot"},
		{"group:wood", "group:wood", "group:wood"}
	}
})

--Desert Outposts
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.000002,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desert_outpost_1a.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.000002,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desert_outpost_1b.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.000002,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desert_outpost_2a.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:desert_sand"},
	biomes = {"desert", "sandstone_desert"},
	sidelen = 50,
	fill_ratio = 0.000002,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/desert_outpost_2b.mts",
	flags = "place_center_x, place_center_z, force_placement",
	rotation = "random",
})

--Flame Obelisks
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:silver_sand"},
	biomes = {"cold_desert"},
	sidelen = 50,
	fill_ratio = 0.000004,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/flame_obelisk.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
})

--Totem Poles
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:dirt_with_rainforest_litter", "default:dry_dirt_with_dry_grass"},
	biomes = {"rainforest", "savanna"},
	sidelen = 50,
	fill_ratio = 0.000009,
	y_max = 300,
	y_min = -5,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/totem_pole.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

--Tradewagon
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone", "default:dirt", "default:dirt_with_grass", "default:dry_dirt_with_dry_grass"},
	biomes = {"grassland", "savanna"},
	sidelen = 100,
	fill_ratio = 0.00001,
	y_max = 300,
	y_min = 7,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/tradewagon.mts",
	flags = "place_center_x, place_center_z, force_placement",
	place_offset_y = 1,
	rotation = "random",
})

--Fortress
minetest.register_decoration({
	deco_type = "schematic",
	place_on = {"default:stone"},
	sidelen = 100,
	fill_ratio = 0.000007,
	y_max = -512,
	y_min = -31000,	
	schematic = minetest.get_modpath("more_structures") .. "/schematics/fortress.mts",
	flags = "place_center_x, place_center_z, force_placement, all_floors",
	place_offset_y = -3,
	rotation = "random",
})

--minetest.register_node("more_structures:villagehouse_1", {
--	description = ("Village House 1"),
--	tiles = {"default_wood.png"},
--	is_ground_content = false,
--	groups = {choppy = 2, oddly_breakable_by_hand = 2},
--	on_place = morestructures.place_house(pos)
--})

--function morestructures.place_house(pos)
--	local node = minetest.get_node(pos)
--	if node.name == "more_structures:villagehouse_1" then
--		local path = minetest.get_modpath("more_structures") .. "/schematics/villagehouse_1.mts",
--		minetest.place_schematic({x = pos.x, y = pos.y, z = pos.z}, path, "0", nil, true)
--	end
--end