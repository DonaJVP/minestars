--MINESTARS X MULTICRAFT & LUANTI

core.log("action", "Registering roof mapgen")

core.register_on_generated(function(vm, minp, maxp, seed)
	if minp.y >= 11000 and maxp.y <= 13000 then
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
					if y == 12000 then
						data[i] = b;
					end
					i = 1 + i;
				end
				idx_y_base = idx_y_base + area.ystride
			end
			idx_z_base = idx_z_base + area.zstride
		end
		vm:set_data(data)
	end
end)