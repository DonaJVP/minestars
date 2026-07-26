function BuildCmdFuncFor(func)
	return function(name, params)
		local prm = params:split(" ")
		local b, pst1 = func(name, unpack(prm))
		if b and pst1 then
			return b, pst1
		else
			return true, core.colorize("lightgreen", "✔"..MineStars.Translator("Done!"))
		end
	end
end