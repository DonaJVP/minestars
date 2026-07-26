local coreregister_ = core.register_craftitem
local coreregister__ = core.register_tool
--[[local coreregister = core.register_node



function core.register_node(nodename, def)
	def.on_place = def.on_place or function(itemstack, placer, pointed_thing) return core.item_place_node(itemstack, placer, pointed_thing) end
	def.on_use = def.on_use or function(i) return i end
	def.on_secondary_use = def.on_secondary_use or function(i) return i end
	print("NODE: "..nodename)
	coreregister(nodename, def)
end--]]

function core.register_craftitem(nodename, def)
	def.on_place = def.on_place or function(itemstack, placer, pointed_thing) return core.item_place(itemstack, placer, pointed_thing) end
	--def.on_use = def.on_use or function() end
	--def.on_secondary_use = def.on_secondary_use or function(i) return i end
	coreregister_(nodename, def)
end

function core.register_tool(nodename, def)
	def.on_place = def.on_place or function(itemstack, placer, pointed_thing) return core.item_place(itemstack, placer, pointed_thing) end
	--def.on_use = def.on_use or function(i) return i end
	--def.on_secondary_use = def.on_secondary_use or function(i) return i end
	coreregister__(nodename, def)
end