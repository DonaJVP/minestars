--Different Name space.
Dimensions = {
	Registered = {},
	Definitions = {}
}

function Dimensions.Register(name, def, self)
	def = def or {}
	def.self = def.self or {}
	def.y_start = def.y_start or 2000
	def.dim_height = def.dim_height or 1000
	def.deep_y = def.deep_y or 200
	def.bedrock_depth = 50
	def.dirt_start = def.y_start + (def.dirt_start or 501)
	def.dirt_depth = def.dirt_depth or 3
	def.ground_limit = def.y_start + (def.ground_limit or 530)
	def.water_depth = def.water_depth or		8
	def.enable_water = def.enable_water == nil
	def.terrain_density = def.terrain_density or	0.4
	def.flatland = def.flatland
	def.cave_threshold = def.cave_threshold or 0.1
	def.map = def.map or {}
	def.map.offset = def.map.offset or 0
	def.map.scale = def.map.scale or 1
	def.map.spread = def.map.spread or {x=100,y=18,z=100}
	def.map.seeddiff = def.map.seeddiff or 24
	def.map.octaves = def.map.octaves or 5
	def.map.persist = def.map.persist or 0.7
	def.map.lacunarity = def.map.lacunarity or 1
	def.map.flags = def.map.flags or "absvalue"
	def.cavemap = def.cavemap or {}
	def.cavemap.offset = def.cavemap.offset or 0
	def.cavemap.scale = def.cavemap.scale or 1
	def.cavemap.spread = def.cavemap.spread or {x=60,y=35,z=60}
	def.cavemap.seeddiff = def.cavemap.seeddiff or 128
	def.cavemap.octaves = def.cavemap.octaves or 5
	def.cavemap.persist = def.cavemap.persist or 0.2
	def.cavemap.lacunarity = def.cavemap.lacunarity or 1.4
	def.cavemap.flags = def.cavemap.flags or "defaults, absvalue"
	def.self.stone = def.stone or "default:stone"
	def.self.dirt = def.dirt or "default:dirt"
	def.self.grass = def.grass or "default:dirt_with_grass"
	def.self.air = def.air or "air"
	def.self.water = def.water or "default:water_source"
	def.self.sand = def.sand or "default:sand"
	def.self.bedrock = def.bedrock or "engine:bedrock"

	def.self.dim_start = def.y_start
	def.self.dim_end = def.y_start + def.dim_height
	def.self.dim_height = def.dim_height
	def.self.ground_limit = def.ground_limit
	def.self.dirt_start = def.dirt_start
	for i,v in pairs(table.copy(def.self)) do
		def.self[i] = minetest.registered_items[v] and minetest.get_content_id(v) or def.self[i]
	end
	for i1, v1 in pairs(table.copy(def)) do
		if i1:sub(-5,-1) == "_ores" then
			for i2,v2 in pairs(v1) do
				local n = minetest.get_content_id(i2)
				def[i1][n] = {}
				local t = type(v2)
				if t == "number" then
					def[i1][n] = {chance=v2}
				elseif t ~="table" then
					error("Error at generating!")
				else
					def[i1][n] = v2
					local ndef = def[i1][n]
					ndef.chance = ndef.chance or 1000
					if ndef.min_heat and not ndef.max_heat then
						ndef.max_heat = 1000
					elseif ndef.max_heat and not ndef.min_heat then
						ndef.min_heat = -1000
					end
				end
				def[i1][i2]=nil
			end
		end
	end
	print(dump(def))
	Dimensions.Registered[name] = def
end
--[[
minetest.register_on_generated(function(minp, maxp, seed)
	-- = minetest.get_mapgen_object("voxelmanip")
	local vm, min, max = VoxelManip()
	local data = vm:get_data()
	core.handle_async(function(dimensions, table_)
		local minp, maxp, seed, data, heat, humidity = unpack(table_)
		for i,d in pairs(dimensions) do
			if minp.y >= d.y_start and maxp.y <= d.y_start+d.dim_height then
				print("wtf")
				local depth = 18
				local height = d.dirt_start
				local ground_limit = d.ground_limit
				local dirt_depth = d.dirt_depth
				local water_depth = d.water_depth
				local cave_threshold = d.cave_threshold
				local lenth = maxp.x-minp.x+1
				local cindx = 1
				local map = PerlinNoiseMap(d.map,{x=lenth,y=lenth,z=lenth}):get_3d_map_flat(minp)
				local cavemap = PerlinNoiseMap(d.cavemap,{x=lenth,y=lenth,z=lenth}):get_3d_map_flat(minp)
				local enable_water = d.enable_water
				local terrain_density = d.terrain_density
				local flatland = d.flatland
				--local heat = 
				--local humidity = 

				local miny = d.y_start
				local maxy = d.y_start + d.dim_height
				local deep_y = d.y_start + d.deep_y --CBN 22/10/2022 Added a param to change the max height of deep stone ores
				local bedrock_depth = d.bedrock_depth

				local dirt = d.self.dirt
				local stone =d.self.stone
				local grass = d.self.grass
				local air = d.self.air
				local water = d.self.water
				local sand = d.self.sand
				local bedrock = d.self.bedrock

				d.self.heat = heat
				d.self.humidity = humidity

				print("PRT1")
				--local vm,min,max = --minetest.get_mapgen_object("voxelmanip")
				print("PRT2")
				local area = VoxelArea:new({MinEdge = minp, MaxEdge = maxp})
				print("PRT3")
				for z=minp.z,maxp.z do
					for y=minp.y,maxp.y do
						local id = area:index(minp.x,y,z)
						for x=minp.x,maxp.x do
							local den = math.abs(map[cindx]) - math.abs(height-y)/(depth*2) or 0
							if y <= miny+bedrock_depth then
								data[id] = bedrock
							elseif y < height and y > miny + bedrock_depth then --CBN 22/10/2022 Fill air pockets
								data[id] = stone
							elseif enable_water and den <= terrain_density and y <= height+d.dirt_depth+1 and y >= height-water_depth  then	--CBN 22/10/2022 Bind water generation
								data[id] = water
								if y+1 == height+d.dirt_depth+1 then -- fix water holes
									data[id+area.ystride] = water
								elseif not (data[id-area.ystride * 2] == water) then --CBN 21/10/2022 Fix intermitent horizontal layers of sand and water
									data[id-area.ystride]=sand
								end
							elseif y >= height and y <= height+dirt_depth then
								data[id] = dirt
								data[id+area.ystride]=grass
							elseif not flatland then
								if y >= height and y<= ground_limit and den >= terrain_density then
									data[id] = dirt
									data[id+area.ystride]=grass
									data[id-area.ystride]=stone
									if den > 1 then
										data[id]=stone
									end
								end
							else
								data[id] = air
							end

							if y < ground_limit and y > miny + bedrock_depth and cavemap[cindx] <= cave_threshold then --CBN 22/10/2022 Cave carving
								data[id] = air
							end

							if d.on_generate then
								data = d.on_generate(d.self,data,id,area,x,y,z) or data
							end

							cindx=cindx+1
							id=id+1
						end
					end
				end

				local node_y = minp.y

				for i1,v1 in pairs(data) do
					if i1%area.ystride == 0 then
						node_y = node_y + 1
					end
					if i1%area.zstride == 0 then --CBN 22/10/2022 data moves in the x axis first, then y axis then z axis, thus when a full zstride has been completed, it goes back to the base of the area
						node_y = minp.y
					end
					local da = data[i1]
					local typ
					if da == air and d.ground_ores and data[i1-area.ystride] == grass then
						typ = "ground"
					elseif da == grass and d.grass_ores then
						typ = "grass"
					elseif da == dirt and d.dirt_ores then
						typ = "dirt"
					elseif da == stone and d.deep_stone_ores and node_y <= miny + deep_y then
						typ = "deep_stone" --CBN 22/10/2022 Added deep stone ores, so that you can set two layers of ores with different chances, to encourage deep mining
					elseif da == stone and d.stone_ores then
						typ = "stone"
					elseif da == air and d.air_ores then
						typ = "air"
					elseif da == water and d.water_ores then
						typ = "water"
					elseif da == sand and d.sand_ores then
						typ = "sand"
					end
					if typ then
						for i2,v2 in pairs(d[typ.."_ores"]) do
							if math.random(1,v2.chance) == 1 and not (v2.min_heat and (heat < v2.min_heat or heat > v2.max_heat)) then
								if v2.chunk then
									for x=-v2.chunk,v2.chunk do
									for y=-v2.chunk,v2.chunk do
									for z=-v2.chunk,v2.chunk do
										local id = i1 + x + (y * area.ystride) + (z * area.zstride)
										if da == data[id] then
											data[id]=i2
										end
									end
									end
									end
								else
									data[i1]=i2
								end
							end
						end
					end
				end
				return data--, minp, maxp
			end
		end
	end, function(data, x1, x2)
		if data then
			local vm = VoxelManip()
			vm:set_data(data)
			vm:write_to_map()
			vm:update_liquids()
		end
	end, Dimensions.Registered, {minp, maxp, seed, data, core.get_heat(minp), core.get_humidity(minp)})
end)--]]

core.register_on_generated(function(minp, maxp, seed)
	local d = Dimensions.Registered.heaven
	do
		if minp.y >= d.y_start and maxp.y <= d.y_start+d.dim_height then
			local depth = 18
			local height = d.dirt_start
			local ground_limit = d.ground_limit
			local dirt_depth = d.dirt_depth
			local water_depth = d.water_depth
			local cave_threshold = d.cave_threshold
			local lenth = maxp.x-minp.x+1
			local cindx = 1
			local map = minetest.get_perlin_map(d.map,{x=lenth,y=lenth,z=lenth}):get_3d_map_flat(minp)
			local cavemap = minetest.get_perlin_map(d.cavemap,{x=lenth,y=lenth,z=lenth}):get_3d_map_flat(minp)
			local enable_water = d.enable_water
			local terrain_density = d.terrain_density
			local flatland = d.flatland
			local heat = minetest.get_heat(minp)
			local humidity = minetest.get_humidity(minp)
			local miny = d.y_start
			local maxy = d.y_start + d.dim_height
			local deep_y = d.y_start + d.deep_y --CBN 22/10/2022 Added a param to change the max height of deep stone ores
			local bedrock_depth = d.bedrock_depth

			local dirt = d.self.dirt
			local stone =d.self.stone
			local grass = d.self.grass
			local air = d.self.air
			local water = d.self.water
			local sand = d.self.sand
			local bedrock = d.self.bedrock

			d.self.heat = heat
			d.self.humidity = humidity

			local vm,min,max = core.get_mapgen_object("voxelmanip")
			local area = VoxelArea:new({MinEdge = min, MaxEdge = max})
			local data = vm:get_data()
			for z=minp.z,maxp.z do
			for y=minp.y,maxp.y do
				local id = area:index(minp.x,y,z)
			for x=minp.x,maxp.x do
				local den = math.abs(map[cindx]) - math.abs(height-y)/(depth*2) or 0
				if y <= miny+bedrock_depth then
					data[id] = bedrock
				elseif y < height and y > miny + bedrock_depth then --CBN 22/10/2022 Fill air pockets
					data[id] = stone
				elseif enable_water and den <= terrain_density and y <= height+d.dirt_depth+1 and y >= height-water_depth  then	--CBN 22/10/2022 Bind water generation
					data[id] = water
					if y+1 == height+d.dirt_depth+1 then -- fix water holes
						data[id+area.ystride] = water
					elseif not (data[id-area.ystride * 2] == water) then --CBN 21/10/2022 Fix intermitent horizontal layers of sand and water
						data[id-area.ystride]=sand
					end
				elseif y >= height and y <= height+dirt_depth then
					data[id] = dirt
					data[id+area.ystride]=grass
				elseif not flatland then
					if y >= height and y<= ground_limit and den >= terrain_density then
						data[id] = dirt
						data[id+area.ystride]=grass
						data[id-area.ystride]=stone
						if den > 1 then
							data[id]=stone
						end
					end
				else
					data[id] = air
				end

				if y < ground_limit and y > miny + bedrock_depth and cavemap[cindx] <= cave_threshold then --CBN 22/10/2022 Cave carving
					data[id] = air
				end

				if d.on_generate then
					data = d.on_generate(d.self,data,id,area,x,y,z) or data
				end

				cindx=cindx+1
				id=id+1
			end
			end
			end

			local node_y = minp.y

			for i1,v1 in pairs(data) do
				if i1%area.ystride == 0 then
					node_y = node_y + 1
				end
				if i1%area.zstride == 0 then --CBN 22/10/2022 data moves in the x axis first, then y axis then z axis, thus when a full zstride has been completed, it goes back to the base of the area
					node_y = minp.y
				end
				local da = data[i1]
				local typ
				if da == air and d.ground_ores and data[i1-area.ystride] == grass then
					typ = "ground"
				elseif da == grass and d.grass_ores then
					typ = "grass"
				elseif da == dirt and d.dirt_ores then
					typ = "dirt"
				elseif da == stone and d.deep_stone_ores and node_y <= miny + deep_y then
					typ = "deep_stone" --CBN 22/10/2022 Added deep stone ores, so that you can set two layers of ores with different chances, to encourage deep mining
				elseif da == stone and d.stone_ores then
					typ = "stone"
				elseif da == air and d.air_ores then
					typ = "air"
				elseif da == water and d.water_ores then
					typ = "water"
				elseif da == sand and d.sand_ores then
					typ = "sand"
				end
				if typ then
					for i2,v2 in pairs(d[typ.."_ores"]) do
						if math.random(1,v2.chance) == 1 and not (v2.min_heat and (heat < v2.min_heat or heat > v2.max_heat)) then
							if v2.chunk then
								for x=-v2.chunk,v2.chunk do
								for y=-v2.chunk,v2.chunk do
								for z=-v2.chunk,v2.chunk do
									local id = i1 + x + (y * area.ystride) + (z * area.zstride)
									if da == data[id] then
										data[id]=i2
									end
								end
								end
								end
							else
								data[i1]=i2
							end
						end
					end
				end
			end
			vm:set_data(data)
			vm:write_to_map()
			vm:update_liquids()
		end
	end
end)



