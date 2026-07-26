--MINESTARS X MULTICRAFT & LUANTI

core.log("action", "Registering 'End of the line' mapgen")

core.register_on_generated(function(vm, minp, maxp, seed)
	if minp.y <= -25000 and maxp.y >= -31000 then
		--data 0x0 [Meta Generation Variable Override]
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
		local cavemap = {
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
		--data 0x1 [Generation Meta]
		local lenth = maxp.x-minp.x+1
		local cavemap = PerlinNoiseMap(cavemap, {x = lenth, y = lenth, z = lenth}):get_3d_map_flat(minp);
		local map = PerlinNoiseMap(map, {x = lenth, y = lenth, z = lenth}):get_3d_map_flat(minp);
		local terrain_density = 0.7;
		local ground_limit = -27020;
		local bedrock_depth = 50;
		local height = -26000;
		local dirt_depth = 5;
		local miny = -31000;
		local maxy = -25000;
		local depth = 18;
		local cindx = 1;
		--data 0x2 [Content DATA]
		local _ = core.get_content_id; -- FUNC
		local grass = _"default:dirt_with_grass";
		local water = _"default:water_source";
		local stone = _"default:stone";
		local dirt = _"default:dirt";
		local b = _"engine:bedrock";
		local air = _"air";
		--generate
		local emin, emax = vm:get_emerged_area();
		local area = VoxelArea:new{MinEdge = emin, MaxEdge = emax};
		local idx_z_base = area:index(minp.x, minp.y, minp.z);
		local data = vm:get_data()
		local b = core.get_content_id("engine:bedrock");
		for z = minp.z, maxp.z do
			local idx_y_base = idx_z_base;
			for y = minp.y, maxp.y do
				local i = idx_y_base;
				for x = minp.x, maxp.x do
					--if y == 12000 then
					--	data[i] = b;
					--end
					local den = math.abs(map[cindx]) - math.abs(height-y)/(depth*2);
					if y <= miny + bedrock_depth then
						data[i] = b;
					elseif (y <= height) and (y > miny + bedrock_depth) then
						data[i] = stone;
					elseif y >= height and y <= (height + dirt_depth) then
						data[i] = dirt;
						data[i + area.ystride] = grass;
					else
						if y >= height and y <= ground_limit and den >= terrain_density then
							data[i] = dirt
							data[i+area.ystride] = grass
							data[i-area.ystride] = stone
							if den > 1 then
								data[i] = stone;
							end
						else
							if y < ground_limit and y > miny + bedrock_depth and cavemap[cindx] <= cave_threshold then
								data[i] = air;
							end
						end
					end
					
					if y == -25000 then
						data[i] = b;
					end
					
					cindx = cindx + 1;
					i = 1 + i;
				end
				idx_y_base = idx_y_base + area.ystride;
			end
			idx_z_base = idx_z_base + area.zstride;
		end
		vm:set_data(data);
	end
end)