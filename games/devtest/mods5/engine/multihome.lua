MineStars.MultiHome = {}

function MineStars.MultiHome.GetHomes(__obj)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local data = core.deserialize((meta:get_string("homes") ~= "" and meta:get_string("homes")) or "return {}")
			print(dump(data))
			return data or {}
		end
	end
end

--home4058 = xyz

function MineStars.MultiHome.ApplyHome(__obj, home)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local data = core.deserialize((meta:get_string("homes") ~= "" and meta:get_string("homes")) or "return {}")
			if data then
				--print(dump(home))
				--local homename = home.name
				--local homepos = home.pos
				data[home.name] = home.pos
				--print(dump(data))
				meta:set_string("homes", core.serialize(data))
			end
		end
	end
end

function MineStars.MultiHome.DeleteHome(__obj, homename)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local data = core.deserialize((meta:get_string("homes") ~= "" and meta:get_string("homes")) or "return {}")
			if data and data[homename] then
				data[homename] = nil
				meta:set_string("homes", core.serialize(data))
				return true
			end
		end
	end
end

--allocate with XP

--do commands

core.register_chatcommand("multihome", {
	params = "<cmd> [name]",
	description = MineStars.Translator("Multi home"),
	func = BuildCmdFuncFor(function(name, cmd, arg1)
		if Player(name) then
			local homes = MineStars.MultiHome.GetHomes(name)
			if cmd == "set" then	
				local xp = MineStars.XP.GetXP(name)
				if (xp > 120) and not (CountTable(homes) == 0) then
					if arg1 then
						MineStars.XP.RemoveXP(name, 120)
						MineStars.MultiHome.ApplyHome(name, {name = arg1, pos = vector.apply(Player(name):get_pos(), math.round)})
					else
						return true, core.colorize("#FF7C7C", X..MineStars.Translator("Atleast specify a name for your home"))	
					end
				elseif CountTable(homes) == 0 then
					MineStars.MultiHome.ApplyHome(name, {name = "home", pos = vector.apply(Player(name):get_pos(), math.round)})
				else
					return true, core.colorize("#FF7C7C", X..MineStars.Translator("Insufficient XP"))
				end
			elseif cmd == "go" then
				if CountTable(homes) == 1 then
					local firsthome = "home" --default
					for NAME in pairs(homes) do
						firsthome = NAME
						break --get the first index
					end
					Player(name):set_pos(homes[firsthome])
				else
					if arg1 then
						if homes[arg1] then
							Player(name):set_pos(homes[arg1])
						else
							return true, core.colorize("#FF7C7C", X..MineStars.Translator("Unknown home @1", arg1))
						end
					else
						return true, core.colorize("#FF7C7C", X..MineStars.Translator("Atleast specify a home!"))
					end
				end
			elseif cmd == "remove" then
				if CountTable(homes) == 1 then
					local firsthome = "home"
					for NAME in pairs(homes) do
						firsthome = NAME
						break
					end
					MineStars.MultiHome.DeleteHome(name, firsthome)
				else
					if arg1 then
						if homes[arg1] then
							MineStars.MultiHome.DeleteHome(name, arg1)
						else
							return true, core.colorize("#FF7C7C", X..MineStars.Translator("Unknown home @1", arg1))
						end
					else
						return true, core.colorize("#FF7C7C", X..MineStars.Translator("Atleast specify a home!"))
					end
				end
			elseif cmd == "list" then
				local homes_ = {}
				for NAME in pairs(homes) do
					table.insert(homes_, NAME)
				end
				return true, core.colorize("lightgreen", arrow..((homes_ == {} and MineStars.Translator("No Home(s)")) or table.concat(homes_, ", ")))
			elseif cmd == "help" or cmd == "ayuda" then
				return true, core.colorize("lightgreen", check..MineStars.Translator("Theres commands: set, list, remove and go"))
			end
		else
			return true, core.colorize("#FF7C7C", X..MineStars.Translator("Must be connected!"))
		end
	end)
})








