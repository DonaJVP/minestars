function stairsplus:register_blocks_text(modname, name, def)
	--Texture
	def.description = def.description.." 32"
	def.tiles = {"[combine:32x32:0,0="..def.tiles[1]..":0,16="..def.tiles[1]..":16,0="..def.tiles[1]..":16,16="..def.tiles[1]}
	core.register_node(":"..modname..":"..name.."_subtxt", def)
	--Crafting
	core.register_craft({
		output = modname..":"..name.."_subtxt",
		recipe = {{modname..":"..name, modname..":"..name, ""}, {modname..":"..name, modname..":"..name, ""}, {"", "", ""}}
	})
	core.register_craft({
		output = modname..":"..name.." 4",
		recipe = {{modname..":"..name.."_subtxt", "", ""}, {"", "", ""}, {"", "", ""}}
	})
end