MineStars.PlayersAdvantage = {}


MineStars.PlayersAdvantage.OnTeleport = function(portaltype, player)
	--check meta
	local meta = player:get_meta()
	if meta:get_int("advantage_"..portaltype) > 0 then
		return
	end
	
	if portaltype == "heaven" then
		MineStars.SO_Effects.DoEffect(player, "quest")
	end
	
	meta:set_int("advantage_"..portaltype, 1)
end















