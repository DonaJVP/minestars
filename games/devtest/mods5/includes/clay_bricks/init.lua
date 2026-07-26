local clay = {
	{"natural", "Natural"},
	{"white", "White"},
	{"grey", "Grey"},
	{"black", "Black"},
	{"red", "Red"},
	{"yellow", "Yellow"},
	{"green", "Green"},
	{"cyan", "Cyan"},
	{"blue", "Blue"},
	{"magenta", "Magenta"},
	{"orange", "Orange"},
	{"violet", "Violet"},
	{"brown", "Brown"},
	{"pink", "Pink"},
	{"dark_grey", "Dark Grey"},
	{"dark_green", "Dark Green"}
}

for i = 1, #clay do
	local name, desc = unpack(clay[i])

	minetest.register_node("clay_bricks:" .. name, {
		description = desc .. " Clay Brick",
		tiles = {"baked_clay_" .. name .. ".png^clay_brick_overlay.png"},
		groups = {cracky = 3, clay_bricks = 1},
		sounds = default.node_sound_defaults(),
	})

	if name ~= "natural" then
		minetest.register_craft{
			output = "clay_bricks:" .. name .. " 8",
			recipe = {
				{"group:clay_bricks", "group:clay_bricks", "group:clay_bricks"},
				{"group:clay_bricks", "group:dye,color_" .. name, "group:clay_bricks"},
				{"group:clay_bricks", "group:clay_bricks", "group:clay_bricks"},
			}
		}
	end

	stairs.register_stair_and_slab("clay_brick_".. name, "clay_bricks:".. name,
			{cracky = 3},
			{"baked_clay_" .. name .. ".png^clay_brick_overlay.png"},
			desc .. " Clay Brick Stair",
			desc .. " Clay Brick Slab",
			default.node_sound_stone_defaults()
	)

	minetest.register_craft({
		output = "clay_bricks:" .. name .. " 4",
		recipe = {
			{"bakedclay:" .. name, "bakedclay:" .. name},
			{"bakedclay:" .. name, "bakedclay:" .. name},
		}
	})

	
end