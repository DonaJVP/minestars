MineStars.OldClients = {}

core.register_on_joinplayer(function(player)
	local info = core.get_player_information(player:get_player_name())
	if info.version_string == "0.4.17.1" or info.version_string == "0.4.17" or info.version_string == "0.4.16" or info.version_string == "0.4.15" then
		MineStars.OldClients[player:get_player_name()] = true
	end
end)

core.register_on_leaveplayer(function(player)
	MineStars.OldClients[player:get_player_name()] = false
end)

--Generative preview
--Don't use static PNG previews, make all at once with ^[combine

-- + {mainpreview}: string, skins:get_preview(); 
-- + {armor}: string, player:get_properties().textures[3]
function MineStars.GetPreviewFromTextures(mainpreview, armor, raw_skin)
	local texture = ""
	local shield =  "blank.png"
	if armor and armor ~= "" and ((armor ~= "blank.png^[mask:onlybody.png") and (armor ~= "blank.png")) then
		--texture = "(" --not concat
		--All are concatenated with '^'
		local textures = armor:split("^")
		--print(dump(textures))
		--lets see.....
		local r_ = {
			LEGGINGS = "",
			CHESTPLATE = "",
			BOOTS = "",
			HELMET = "",
			--SHIELD = ""
		}
		
		--local found_helmet = {false, ""}
		
		for _, armor_string in pairs(textures) do
			if armor_string:match("chestplate") then
				r_.CHESTPLATE = "([combine:16x32:-16,-12="..armor_string..")" --bugs
			elseif armor_string:match("helmet") then
				r_.HELMET = "([combine:16x32:-36,-7="..armor_string..")"
				--found_helmet = {true, armor_string}
			elseif armor_string:match("leggings") then
				r_.LEGGINGS = "([combine:16x32:-4,0="..armor_string..")" --two handled 1
			elseif armor_string:match("boots") then
				r_.BOOTS = "([combine:16x32:-16,28="..armor_string..")"
			--elseif armor_string:match("shield") then
			--	shield = armor_string
			end
		end
		
		--print(dump(r_))
		
		local eval_t = {}
		for _, armor_ in pairs(r_) do
			--print(armor_, _)
			if armor_ ~= "" and _ ~= "HELMET" then
				--print("Asserted!")
				table.insert(eval_t, armor_)
			end
		end
		
		if r_.HELMET ~= "" then
			table.insert(eval_t, r_.HELMET)
		end
		
		--print(dump(eval_t))
		
		for _, evaluated in pairs(eval_t) do
			texture = texture .. evaluated .. ((eval_t[_ + 1] and "^") or "")
		end
		texture = texture
		--print("GOT!: "..texture)
		if texture ~= "" then
			return mainpreview.."^((("..texture..")^[resize:64x128)^[mask:armor_transform.png)", shield
		end
		--print(texture)
	end
	--turn for preview + armor
	return mainpreview, shield
end
























