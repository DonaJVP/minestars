--local textures = {
	local night = {
		"DarkStormyUp.jpg",
		"DarkStormyDown.jpg",
		"DarkStormyFront.jpg",
		"DarkStormyBack.jpg",
		"DarkStormyLeft.jpg",
		"DarkStormyRight.jpg",
	}
	local day = {
		"ThickCloudsWaterUp.jpg",
		"ThickCloudsWaterDown.jpg",
		"ThickCloudsWaterFront.jpg",
		"ThickCloudsWaterBack.jpg",
		"ThickCloudsWaterLeft.jpg",
		"ThickCloudsWaterRight.jpg",
		--Heaven
		--[[[2] = {
			"TropicalSunnyDayUp.jpg",
			"TropicalSunnyDayDown.jpg",
			"TropicalSunnyDayFront.jpg",
			"TropicalSunnyDayBack.jpg",
			"TropicalSunnyDayLeft.jpg",
			"TropicalSunnyDayRight.jpg",
		}--]]
	}
--}

local active = ":" -- night:0="Sector Night, but using principal table", day:2="Sector Day, using the table N*2"

local full_day = 0
local full_night = 0

function MineStars.UpdateSkybox(dtime, players)
	local time = core.get_timeofday()
	--[[
		OPACITY:
		DAY: 255, 0
		MIDDLE: [90, 150] {night, day}
		NIGHT: 255, 0
		
		GETTIMEOFDAY:
		0: 0:00
		0.5: 12:00
		1: 23:59
	--]]
	--[[if time > 0.1 and time < 0.74 then
		full_night = full_night + (time*dtime)
	else
		full_night = full_night - (time*dtime)
	end
	
	if time > 0.2 and time < 0.85 then
		full_day = full_day + (time*dtime)
	else
		full_day = full_day - (time*dtime)
	end--]]
	
	local valueNIGHT = (time > 0.7 and time < 0.2 and time*dtime) or -time*dtime
	local valueDAY = (time > 0.2 and time < 0.85 and time*dtime) or -time*dtime
	
	--verify
	valueNIGHT = (not (valueNIGHT > 255)) and valueNIGHT or 255
	valueDAY = (not (valueDAY > 255)) and valueDAY or 255
	
	for _, p in pairs(players) do
		local instruct = p:get_sky()
		p:set_sky({
			base_color = instruct.base_color,
			type = "skybox",
			textures = {
				"("..night[1].."^[opacity:"..valueNIGHT..")".."^("..day[1].."^[opacity:"..valueDAY..")",
				"("..night[2].."^[opacity:"..valueNIGHT..")".."^("..day[2].."^[opacity:"..valueDAY..")",
				"("..night[3].."^[opacity:"..valueNIGHT..")".."^("..day[3].."^[opacity:"..valueDAY..")",
				"("..night[4].."^[opacity:"..valueNIGHT..")".."^("..day[4].."^[opacity:"..valueDAY..")",
				"("..night[5].."^[opacity:"..valueNIGHT..")".."^("..day[5].."^[opacity:"..valueDAY..")",
				"("..night[6].."^[opacity:"..valueNIGHT..")".."^("..day[6].."^[opacity:"..valueDAY..")",
			},
			clouds = true,
		})
	end
end








