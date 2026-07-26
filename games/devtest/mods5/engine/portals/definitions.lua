--[[Dimensions.Definitions.Ores = {
	["default:stone_with_coal"] = 200,
	["default:stone_with_iron"] = 400,
	["default:stone_with_copper"] =  500,
	["default:stone_with_gold"] = 2000,
	["default:stone_with_mese"] = 10000,
	["default:stone_with_diamond"] = 20000,
	["default:mese"] = 40000,
	["default:gravel"] = {chance=3000,chunk=2,}
}

Dimensions.Definitions.Plants = {
	["flowers:mushroom_brown"] = 1000,
	["flowers:mushroom_red"] = 1000,
	["flowers:mushroom_brown"] = 1000,
	["flowers:rose"] = 1000,
	["flowers:tulip"] = 1000,
	["flowers:dandelion_yellow"] = 1000,
	["flowers:geranium"] = 1000,
	["flowers:viola"] = 1000,
	["flowers:dandelion_white"] = 1000,
	["default:junglegrass"] = 2000,
	["default:papyrus"] = 2000,
	["default:grass_3"] = 10,
	--["multidimensions:tree"] = 1000,
	--["multidimensions:aspen_tree"] = 1000,
	--["multidimensions:pine_tree"] = 1000,
}--]]

core.register_node("engine:bedrock", {
	description = "Bedrock",
	tiles = {"bedrock.png"},
	groups = {immortal = 1, not_in_creative_inventory = 1},
	paramtype = "light",
	sunlight_propagates = true,
	drop = "",
	diggable = false,
	sounds = (default and default.node_sound_stone_defaults()) or nil,
})