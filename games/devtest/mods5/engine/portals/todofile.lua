--MINESTARS X MULTICRAFT OBJECT FILE

--assert(not core.MineStars_Server) --Defined in C++

core.log("action", "Registering heaven mapgen")

--Multithreaded mapgen on c++
core.register_on_generated(function(vm, minp, maxp, seed)
	if minp.y >= 2000 and maxp.y <= 3000 then
		--Dump values [debug]
		--print(dump(minp))
		--print(dump(maxp))
		--print(dump(vm))
		--print(dump(seed))
		--Alias for better programming
		local c = core.get_content_id
		--Around data...
		local map = {
			offset = 0,
			flags = "absvalue",
			persist = 0.7,
			octaves = 5,
			scale = 1,
			lacunarity = 1,
			spread = {
				y = 18,
				x = 100,
				z = 100
			},
			seeddiff = 24
		}
		local cmap = {
			offset = 0,
			flags = "defaults, absvalue",
			persist = 0.2,
			octaves = 5,
			scale = 1,
			lacunarity = 1.4,
			spread = {
				y = 35,
				x = 60,
				z = 60
			},
			seeddiff = 128
		}
		local depth = 18
		local height = 2501
		local ground_limit = 2530
		local dirt_depth = 3
		local water_depth = 8
		local cave_threshold = 0.1
		local lenth = maxp.x-minp.x+1
		local cindx = 1
		local map = PerlinNoiseMap(map, {x=lenth,y=lenth,z=lenth}):get_3d_map_flat(minp)
		local cavemap = PerlinNoiseMap(cmap, {x=lenth,y=lenth,z=lenth}):get_3d_map_flat(minp)
		local enable_water = true
		local terrain_density = 0.4
		local flatland = nil
		local heat = core.get_heat(minp)
		local miny = 2000
		local maxy = 3000
		local deep_y = 2200
		local bedrock_depth = 50
		local dirt = c"engine:heaven_dirt"
		local stone = c"engine:heaven_stone"
		local grass = c"engine:heaven_grass"
		local air = c"air"
		local water = c"engine:heaven_water"
		local sand = c"engine:heaven_sand"
		local bedrock = c"engine:bedrock"
		local ores = {
			sand_ores = {
				[c"default:clay"] = {chunk = 2, chance = 5000}
			},
			ground_ores = {
				[c"flowers:mushroom_brown"] = 1000,
				[c"flowers:mushroom_red"] = 1000,
				[c"flowers:mushroom_brown"] = 1000,
				[c"flowers:rose"] = 1000,
				[c"flowers:tulip"] = 1000,
				[c"flowers:dandelion_yellow"] = 1000,
				[c"flowers:geranium"] = 1000,
				[c"flowers:viola"] = 1000,
				[c"flowers:dandelion_white"] = 1000,
				[c"default:junglegrass"] = 2000,
				[c"default:papyrus"] = 2000,
				[c"default:grass_3"] = 10,
			},
			stone_ores = {
				[c"default:stone_with_coal"] = 200,
				[c"default:stone_with_iron"] = 400,
				[c"default:stone_with_copper"] =  500,
				[c"default:stone_with_gold"] = 2000,
				[c"default:stone_with_mese"] = 10000,
				[c"default:stone_with_diamond"] = 20000,
				[c"default:mese"] = 40000,
				[c"default:gravel"] = {chance=3000,chunk=2,},
				[c"sunstone:sunstone_ore"] = 30000,
				[c"moonstone:moonstone_ore"] = 30000
			}
		}
		--Generate
		--print("0")
		local emin, emax = vm:get_emerged_area()
		local area = VoxelArea:new{MinEdge = emin, MaxEdge = emax}
		local data = vm:get_data()
		local idx_z_base = area:index(minp.x, minp.y, minp.z)
		
		for z = minp.z, maxp.z do
			local idx_y_base = idx_z_base
			for y = minp.y, maxp.y do
				local i = idx_y_base
				for x = minp.x, maxp.x do
					local id = i
					--Content
					local den = math.abs(map[cindx]) - math.abs(height-y)/(depth*2) or 0
					if y <= miny + bedrock_depth then
						data[id] = bedrock
					elseif y < height and y > miny + bedrock_depth then
						data[id] = stone
					elseif den <= terrain_density and y <= height+dirt_depth+1 and y >= height-water_depth then
						data[id] = water
						if (y + 1) == (height + dirt_depth + 1) then
							data[id + area.ystride] = water
						elseif not (data[id-area.ystride * 2] == water) then
							data[id - area.ystride] = sand
						end
					elseif y >= height and y <= (height + dirt_depth) then
						data[id] = dirt
						data[id + area.ystride] = grass
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
					--Ground Content
					if y < ground_limit and y > miny + bedrock_depth and cavemap[cindx] <= cave_threshold then
						data[id] = air
					end
					--Data
					cindx = cindx + 1
					--id = id + 1
					i = 1 + i
				end
				idx_y_base = idx_y_base + area.ystride
			end
			idx_z_base = idx_z_base + area.zstride
		end
		
		--[[
		for z = minp.z, maxp.z do
			for y = minp.y, maxp.y do
				for x = minp.x, maxp.x do
					local id = area:index(x, y, z)
					--Content
					local den = math.abs(map[cindx]) - math.abs(height-y)/(depth*2) or 0
					if y <= miny + bedrock_depth then
						data[id] = bedrock
					elseif y < height and y > miny + bedrock_depth then
						data[id] = stone
					elseif den <= terrain_density and y <= height+dirt_depth+1 and y >= height-water_depth then
						data[id] = water
						if (y + 1) == (height + dirt_depth + 1) then
							data[id + area.ystride] = water
						elseif not (data[id-area.ystride * 2] == water) then
							data[id - area.ystride] = sand
						end
					elseif y >= height and y <= (height + dirt_depth) then
						data[id] = dirt
						data[id + area.ystride] = grass
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
					--Ground Content
					if y < ground_limit and y > miny + bedrock_depth and cavemap[cindx] <= cave_threshold then
						data[id] = air
					end
					--Data
					cindx = cindx + 1
					--id = id + 1
				end
			end
		end--]]
		
		--local idx_z_base = area:index(minp.x, minp.y, minp.z)
		
		
		--
		local node_y = minp.y
		for i1, v1 in pairs(data) do
			if (i1 % area.ystride) == 0 then
				node_y = node_y + 1
			end
			--bugfx
			if (i1 % area.zstride) == 0 then
				node_y = minp.y
			end
			local da = data[i1]
			local typ
			if da == air and ores.ground_ores and data[i1 - area.ystride] == grass then --x
				typ = "ground"
			elseif da == stone and ores.stone_ores then
				typ = "stone"
			elseif da == sand and ores.sand_ores then
				typ = "sand"
			end
			if typ then
				for i2, v2 in pairs(ores[typ.."_ores"]) do
					if type(v2) == "table" and (math.random(1,v2.chance) == 1 and not (v2.min_heat and (heat < v2.min_heat or heat > v2.max_heat))) or type(v2) == "number" and (math.random(1, v2) == 1) then
						if type(v2) == "table" and v2.chunk then
							for x=-v2.chunk,v2.chunk do
								for y=-v2.chunk,v2.chunk do
									for z=-v2.chunk,v2.chunk do
										local id = i1 + x + (y * area.ystride) + (z * area.zstride)
										if da == data[id] then
											data[id] = i2
										end
									end
								end
							end
						else
							data[i1] = i2
						end
					end
				end
			end
		end
		--print(dump(data))
		vm:set_data(data)
		--vm:write_to_map()
	end
end)



















