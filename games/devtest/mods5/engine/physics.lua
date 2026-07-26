--Physics
MineStars.Physics = {
	Overriders = {},
	CanSetOtherOverrider = {
		--[[
		Logiki = {bool=false,reason="FRAMEWORK_ENGINE:Shields"}
		--]]
	},
	Default = {
		gravity = 1,
		speed = 1,
		jump = 1,
	},
	Registered = {},
	DefaultPhysics = {},
}
--[[
function MineStars.Physics.Set(p, n, v, b, n_, force)
	if Name(p) then
		if MineStars.Physics.CanSetOtherOverrider[Name(p)] then
			return false
		else
			if b then
				MineStars.Physics.CanSetOtherOverrider[Name(p)] = {
					bool = false,
					reason = n_,
				}
			end
			local physics = p:get_physics_override()
			print(dump(physics))
			if not force then
				physics[n] = v.n + ((physics[n] ~= 1 and physics[n]) or 1)
			else
				v2 = {n = v.m, m = physics[n], true}
				physics[n] = v.m
			end
			MineStars.Physics.Overriders[Name(p)][n_] = {method = n, value = v2}
			print(dump(physics))
			p:set_physics_override(physics)
			return true
		end
	end
end

function MineStars.Physics.Remove(p, n_)
	if Name(p) then
		if MineStars.Physics.Overriders[Name(p)][n_] then
			if MineStars.Physics.CanSetOtherOverrider[Name(p)] and MineStars.Physics.CanSetOtherOverrider[Name(p)].reason == n_ then
				MineStars.Physics.CanSetOtherOverrider[Name(p)] = nil
			end
			local physics = p:get_physics_override()
			if not MineStars.Physics.Overriders[Name(p)][n_][3] then
				physics[MineStars.Physics.Overriders[Name(p)][n_].method] = physics[MineStars.Physics.Overriders[Name(p)][n_].method] + MineStars.Physics.Overriders[Name(p)][n_].value.m
			else
				physics[MineStars.Physics.Overriders[Name(p)][n_].method] = MineStars.Physics.Overriders[Name(p)][n_].value.m
			end
			p:set_physics_override(physics)
			MineStars.Physics.Overriders[Name(p)][n_] = nil
		else
			return false
		end
	end
end
--]]
core.register_on_joinplayer(function(p)
	MineStars.Physics.Registered[Name(p)] = {}
	--MineStars.Physics.Overriders[Name(p)] = {}
	--MineStars.Physics.CanSetOtherOverrider[Name(p)] = nil
end)

core.register_on_leaveplayer(function(p)
	MineStars.Physics.Registered[Name(p)] = nil
	--MineStars.Physics.Overriders[Name(p)] = nil
	--MineStars.Physics.CanSetOtherOverrider[Name(p)] = nil
end)

function GetPhysics(player) -- to be overriden by other mods!
	return MineStars.Physics.DefaultPhysics[Name(player)]
end

function MineStars.Physics.Apply(p)
	core.after(0.1, function(p)
		if (not p) or not p:get_player_name() then
			return
		end
		local physics = table.copy(MineStars.Physics.DefaultPhysics[Name(p)])
		--print(dump(physics))
		local prefix_ot
		if not Name(p) then
			return
		end
		if not MineStars.Physics.Registered[Name(p)] then
			return
		end
		MineStars.Physics.Registered[Name(p)] = MineStars.Physics.Registered[Name(p)] or {}
		for name, data in pairs(MineStars.Physics.Registered[Name(p)]) do
			--[[for statname, tabled in pairs(data) do
				if tabled.overrideother then
					
				end
				
			end--]]
			--print(name, data)
			if data.overrideother then
				prefix_ot = name
				break
			end
			
		end
		if prefix_ot then
			for statname, datatable in pairs(MineStars.Physics.Registered[Name(p)][prefix_ot]) do
				if type(datatable) == "number" then
					physics[statname] = datatable
				end
			end
			p:set_physics_override(physics)
		end
		--no other
		for name, data in pairs(MineStars.Physics.Registered[Name(p)]) do
			if type(data) == "number" then
				physics[name] = data
			end
		end
		core.log("info", "MineStars: At Physics => Apply: Physics for player '"..Name(p).."': "..dump(physics))
		p:set_physics_override(physics)
		--print(dump(physics))
	end, p)
	return true
end

function MineStars.Physics.Set(p, m, v, n_, force)
	local player = Player(p)
	if player then
		MineStars.Physics.Registered[Name(p)][n_] = m
		return MineStars.Physics.Apply(player)
	end
end

function MineStars.Physics.Remove(p, n_)
	local player = Player(p)
	if player and Name(p) and (n_) and MineStars.Physics.Registered[Name(p)] then
		MineStars.Physics.Registered[Name(p)][n_] = nil
		MineStars.Physics.Apply(player)
		return true
	end
end
