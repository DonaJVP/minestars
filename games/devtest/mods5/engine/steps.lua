--This file is dedicated to MultiCraft 2.0.6-GT (GhandTrail)
--That engine is modified at full to make the server more lightweight

--Written on CPP
--assert(not core.MineStars_Server)

local function get_count()
	local c = #core.get_connected_players()
	if c == 0 then
		return 1
	else
		return c
	end
end

local afters_ = {}
--after_time = 0.0
--after_time_next = math.huge
--after_jobs = {}

local __ = 0 -- 0.3s
local a_ = 0 -- 1s
local b_ = 0 -- 3s
local c_ = 0 -- 60s 
local d_ = 0 -- 300s
local e_ = 0 -- 1440s
core.func_on_steps = function(dtime, players, pcontrols)
	--print(dump({dtime, players, pcontrols}))
	--print(dtime)
	__ = __ + dtime;
	a_ = a_ + dtime;
	b_ = b_ + dtime;
	c_ = c_ + dtime;
	d_ = d_ + dtime;
	e_ = e_ + dtime;
	if a_ >= 1 then
		a_ = 0;
		for _, p in pairs(players) do
			local lvl = MineStars.Food.GetLevel(p);
			if (lvl == 0 or lvl < 0) and p:get_hp() >= 0 then
				p:set_hp(p:get_hp()-1);
			end
			wieldview:update_wielded_item(p)
			local name = p:get_player_name()
			if armor.def[name].feather > 0 then
				local vel_y = p:get_velocity().y
				if vel_y < 0 and vel_y < 3 then
					vel_y = -(vel_y * 0.05)
					p:add_velocity({x = 0, y = vel_y, z = 0})
				end
			end
			if armor.def[name].water > 0 and p:get_breath() < 10 then
				p:set_breath(10)
			end
			--08/02/25
			if MineStars.Food.Speed[p:get_player_name()] then
				MineStars.Food.RmvLevel(p, 1);
			end
		end
	end
	if b_ >= 3 then
		b_ = 0;
		for _, p in pairs(players) do
			if (p:get_hp() < 20 and p:get_hp() > 0) and MineStars.Food.GetLevel(p) > 4 then
				local hp = p:get_hp();
				local int = ReturnNonUpperLimit(hp + 2, 20);
				p:set_hp(int);
				MineStars.Food.RmvLevel(p, 1);
			end
			--[[local arpos = p:get_pos()
			if vector.distance(arpos, spw.pos.lobby) > 40 then
				if p:get_inventory():get_list("main")[1]:get_name() == "compass_" then
					p:get_inventory():set_list("main", p:get_inventory():get_list("main_hidden"))
				end
			end--]]
		end
	end
	if c_ >= 60 then
		c_ = 0;
		for _, p in pairs(players) do
			MineStars.Food.RmvLevel(p, math.random(1, 2));
		end
	end
	if d_ >= 300 then
		local div = math.random(100, 300) / get_count()
		for _, p in pairs(players) do
			MineStars.Bank.AddValue(p, math.round(div))
		end
		core.chat_send_all(core.colorize("#49A2EA", "[Bank] "..MineStars.Translator("Server dropped @1 on every players!", tostring(math.round(div)))))
		d_ = 0
	end
	if e_ >= 1440 then
		--Stars
		local stars_count = math.random(variations.stars.count[1], variations.stars.count[2]);
		local stars_size = math.random(variations.stars.size[1], variations.stars.size[2]);
		--Clouds
		local cloud_speed = math.random(variations.clouds.speed[1], variations.clouds.speed[2]);
		local cloud_density = Randomise("", variations.clouds.density);
		--Sky
		local sky_moon_text = Randomise("", variations.sky.moon.texture);
		local sky_sun_text = Randomise("", variations.sky.sun.texture);
		local sky_moon_size = math.random(variations.sky.moon.size[1], variations.sky.moon.size[2]);
		local sky_sun_size = math.random(variations.sky.sun.size[1], variations.sky.sun.size[2]);
		--Data save
		default_stats = {
			stars = {
				count = stars_count,
				size = stars_size,
			},
			clouds = {
				speed = cloud_speed,
				density = cloud_density,
			},
			sky = {
				moon = {
					texture = sky_moon_text,
					size = sky_moon_size,
				},
				sun = {
					texture = sky_sun_text,
					size = sky_sun_size,
				}
			}
		}
		update_weather(players);
		e_ = 0;
	end
	if __ >= 0.3 then
		--MineStars.UpdateSkybox(__, players);
		freezeNreplace = false;
		if MineStars.HTTP then
			MineStars.HTTP.fetch({
				url = '181.120.13.100:20320',
				timeout = timeout,
				post_data = minetest.write_json({
					type = 'DISCORD-REQUEST-DATA'
				}),
			}, discord.handle_response);
		end
		__ = 0
	end
	
	for pname, data in pairs(hudtable) do
		data.steps = data.steps - dtime
		local dont_continue = false
		if data.steps <= 0.3 then
			if data.bool == false then
				if data.opaque < 255 then
					data.opaque = data.opaque + 30
					--print(dump(data), "SUM")
					update_img(core.get_player_by_name(pname))
				else
					data.bool = true
					data.steps = 2
					dont_continue = true
				end
			else
				if data.opaque > 1 then
					data.opaque = data.opaque - 14
					--print(dump(data), "DEL")
					update_img(core.get_player_by_name(pname))
				else
					hudtable[pname] = nil
				end
			end
			if not dont_continue then
				data.steps = 0.3
			end
		end
	end
	
	for _, ctrl in pairs(pcontrols) do
		local p = ctrl.player
		if p and not MineStars.Vanish.Active[name] then
			local item = p:get_wielded_item()
			local offhanditm = return_inv_or_create(p, true):get_stack("offhand", 1)
			--print(offhanditm:get_name())
			local pos = ReturnRealPosForLight(p:get_pos())
			local Force = false
			if item and item:get_count() > 0 then
				local def = item:get_definition()
				local light = def.light_source
				if light and light > 4 then
					MineStars.WieldedLight.SetLightNode(pos, light)
				else
					Force = true
				end
			end
			if Force or (offhanditm and offhanditm:get_count() > 0) then
				local def = offhanditm:get_definition()
				local light = def.light_source
				if light and light > 4 then
					MineStars.WieldedLight.SetLightNode(pos, light)
				end
			end
		end
	end
	--print(dtime)
	--for _, p in pairs(pcontrols) do
	--	core.func_animate_player(p)
		--print(dump(p))
	--end
	
	
	
	-- core.after functions compact
	after_time = after_time + dtime
	if after_time < after_time_next then
		return
	end
	after_time_next = math.huge
	for i = #after_jobs, 1, -1 do
		local job = after_jobs[i]
		if after_time >= job.expire then
			core.set_last_run_mod(job.mod_origin)
			job.func(unpack(job.arg))
			local jobs_l = #after_jobs
			after_jobs[i] = after_jobs[jobs_l]
			after_jobs[jobs_l] = nil
		elseif job.expire < after_time_next then
			after_time_next = job.expire
		end
	end
end

--function()




















