variations = {
	stars = {
		count = {1000, 14000},
		size = {1, 3},
	},
	clouds = {
		speed = {1, 20},
		density = {0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7}
	},
	sky = {
		moon = {
			texture = {"moon.png", "moonstone_block.png"},
			size = {2, 3},
		},
		sun = {
			texture = {"sun.png", "sunstone_block.png"},
			size = {2, 5},
		}
	}
}

default_stats = {
	stars = {
		count = 1200,
		size = 2,
	},
	clouds = {
		speed = 3,
		density = 0.3,
	},
	sky = {
		moon = {
			texture = "moon.png",
			size = 3,
		},
		sun = {
			texture = "sun.png",
			size = 2
		}
	}
}

function update_weather(players)
	for _, player in pairs(players) do
		player:set_stars{
			size = default_stats.stars.size,
			count = default_stats.stars.count
		}
		player:set_moon{
			texture = default_stats.sky.moon.texture,
			size = default_stats.sky.moon.size,
		}
		player:set_sun{
			texture = default_stats.sky.sun.texture,
			size = default_stats.sky.sun.size,
		}
		player:set_clouds{
			speed = default_stats.clouds.speed,
			density = default_stats.clouds.density,
		}
	end
end
--[[
local tempo = 0
core.register_globalstep(function(dt)
	tempo = tempo + dt
	if tempo >= 1440 then --1440
		--Tempo
		tempo = 0
		--Stars
		local stars_count = math.random(variations.stars.count[1], variations.stars.count[2])
		local stars_size = math.random(variations.stars.size[1], variations.stars.size[2])
		--Clouds
		local cloud_speed = math.random(variations.clouds.speed[1], variations.clouds.speed[2])
		local cloud_density = Randomise("", variations.clouds.density)
		--Sky
		local sky_moon_text = Randomise("", variations.sky.moon.texture)
		local sky_sun_text = Randomise("", variations.sky.sun.texture)
		local sky_moon_size = math.random(variations.sky.moon.size[1], variations.sky.moon.size[2])
		local sky_sun_size = math.random(variations.sky.sun.size[1], variations.sky.sun.size[2])
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
		update_weather()
	end
end)--]]

core.register_on_joinplayer(function(player)
	--Use static stats for sky
	player:set_stars{
		size = default_stats.stars.size,
		count = default_stats.stars.count
	}
	player:set_moon{
		texture = default_stats.sky.moon.texture,
		size = default_stats.sky.moon.size,
	}
	player:set_sun{
		texture = default_stats.sky.sun.texture,
		size = default_stats.sky.sun.size,
	}
	player:set_clouds{
		speed = default_stats.clouds.speed,
		density = default_stats.clouds.density,
		thickness = 64,
	}
end)





















