MineStars.Labels = {
	RegisteredLabels = {}
}

local fastaccess = {}

function MineStars.Labels.GetLabel(__obj)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			if not fastaccess[Name(__obj)] then
				local str = meta:get_string("label")
				if str ~= "" then
					fastaccess[Name(__obj)] = str
					return str
				end
			else
				return fastaccess[Name(__obj)]
			end
		end
	end
	return ""
end

function MineStars.Labels.SetLabel(__obj, label)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			if MineStars.Labels.Get(label) then
				meta:set_string("label", label)
				fastaccess[Name(__obj)] = label
			end
		end
	end
end

function MineStars.Labels.GetLabelColor(label)
	return (MineStars.Labels.RegisteredLabels[label] and MineStars.Labels.RegisteredLabels[label].ColorSTR) or "#FFFFFF"
end

function MineStars.Labels.Get(name)
	return MineStars.Labels.RegisteredLabels[name]
end

function MineStars.Labels.RegisterLabel(name, def)
	MineStars.Labels.RegisteredLabels[name] = def
end

MineStars.Labels.RegisterLabel("Builder", {
	ColorSTR = "#009292",
})
MineStars.Labels.RegisterLabel("Moderator", {
	ColorSTR = "#FF9828",
})
MineStars.Labels.RegisterLabel("Guardian", {
	ColorSTR = "#FFD528",
})

core.register_chatcommand("set_label", {
	params = "<player> <label>",
	description = MineStars.Translator"Set label",
	func = BuildCmdFuncFor(function(name, player, label)
		if player and label then
			if IsOnline(player) then
				if MineStars.Labels.Get(label) then
					MineStars.Labels.SetLabel(player, label)
				end
			end
		end
	end)
})















