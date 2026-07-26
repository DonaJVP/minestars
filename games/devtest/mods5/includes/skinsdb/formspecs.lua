local S = minetest.get_translator("skinsdb")
local ui = minetest.global_exists("unified_inventory") and unified_inventory

if ui then
	ui.imgscale = 1
end

function skins.get_formspec_context(player)
	if player then
		local playername = player:get_player_name()
		skins.ui_context[playername] = skins.ui_context[playername] or {}
		return skins.ui_context[playername]
	else
		return {}
	end
end

-- Show skin info
function skins.get_skin_info_formspec(skin, perplayer_formspec)
	local texture = skin:get_texture()
	local m_name = skin:get_meta_string("name")
	local m_author = skin:get_meta_string("author")
	local m_license = skin:get_meta_string("license")
	local m_format = skin:get_meta("format")
	-- overview page
	local raw_size = m_format == "1.8" and "3,3" or "3,2"

	local lxoffs = 0.8
	local cxoffs = 2
	local rxoffs = 5.5
	local ui_enabled = false

	if type(perplayer_formspec) == "table" then -- we're using Unified Inventory
		lxoffs = 1.5
		cxoffs = 3.75
		rxoffs = 7.5
		ui_enabled = true
	end


	local formspec = (ui_enabled and "image[4.7,2.75;1.6,3;"..skin:get_preview().."]") or "" -- OBJ
	
	local image = skin:get_texture()
	--local formspec = ""
	if m_format == "1.8" then
		--formspec = formspec.."label[2.5,2.5;"..S("Model")..":]"
		formspec = formspec..((ui_enabled and "model[6.8,2.5;2.5,4;;skinsdb_3d_armor_character_5.b3d;blank.png," .. image .. ",blank.png,blank.png;-20,-150;false;true;0,45]") or "model[4.8,0.5;1.9,3.2;;skinsdb_3d_armor_character_5.b3d;blank.png," .. image .. ",blank.png,blank.png;-20,-150;false;true;0,45]")
	else
		--formspec = formspec.."label[2.5,2.5;"..S("Model")..":]"
		formspec = formspec..((ui_enabled and "model[6.8,2.5;2.5,4;;skinsdb_3d_armor_character_5.b3d;" .. image .. ",blank.png,blank.png,blank.png;-20,-150;false;true;0,45]") or "model[4.8,0.5;1.9,3.2;;skinsdb_3d_armor_character_5.b3d;" .. image .. ",blank.png,blank.png,blank.png;-20,-150;false;true;0,45]")
	end
	
	local datatable = {
		[1] = "0,0.9",
		[2] = "0,1.3",
		[3] = "0,1.7"
	}
	
	local unified = {
		[1] = "0.5,2.9",
		[2] = "0.5,3.3",
		[3] = "0.5,3.7"
	}
	
	local use_unified = type(perplayer_formspec) == "table"
	
	--if texture then
	--	formspec = formspec.."label["..rxoffs..",2.5;"..S("Raw texture")..":]"
	--	.."image["..rxoffs..",3;"..raw_size..";"..texture.."]"
	--end
	if m_name ~= "" then
		formspec = formspec.."label["..((use_unified and unified[1]) or datatable[1])..";"..S("Name")..": "..minetest.formspec_escape(m_name).."]"
	end
	if m_author ~= "" then
		formspec = formspec.."label["..((use_unified and unified[2]) or datatable[2])..";"..S("Author")..": "..minetest.formspec_escape(m_author).."]"
	end
	if m_license ~= "" then
		formspec = formspec.."label["..((use_unified and unified[3]) or datatable[3])..";"..S("License")..": "..minetest.formspec_escape(m_license).."]"
	end
	if perplayer_formspec then
		formspec = formspec .. "label[8.4,"..(perplayer_formspec.formspec_y + 7.9)..";" .. MineStars.Translator("Trash:") .. "]"
		formspec = formspec .. "list[detached:trash;main;8.5,"..(perplayer_formspec.formspec_y + 8.3)..";1,1;]"
		formspec = formspec .. "background[8.5,"..(perplayer_formspec.formspec_y + 8.3)..";1,1;ui_single_slot.png]"
		formspec = formspec .. "label[8.3,"..(perplayer_formspec.formspec_y + 5.9)..";" .. MineStars.Translator("Offhand:") .. "]"
		formspec = formspec .. "list[current_player;offhand;8.5,"..(perplayer_formspec.formspec_y + 6.3)..";1,1;]"
		formspec = formspec .. "background[8.5,"..(perplayer_formspec.formspec_y + 6.3)..";1,1;ui_single_slot.png]"
		formspec = formspec .. "background[0,"..(perplayer_formspec.formspec_y + 6.3)..";8,4;ui_main_inventory.png]"
	end
	return formspec
end
--[[
function skins.get_skin_info_formspec(skin, perplayer_formspec)
	local texture = skin:get_texture()
	local m_name = skin:get_meta_string("name")
	local m_author = skin:get_meta_string("author")
	local m_license = skin:get_meta_string("license")
	local m_format = skin:get_meta("format")
	-- overview page
	local raw_size = m_format == "1.8" and "2,2" or "2,1"

	local lxoffs = 0.8
	local cxoffs = 2
	local rxoffs = 5.5

	if type(perplayer_formspec) == "table" then -- we're using Unified Inventory
		lxoffs = 1.5
		cxoffs = 3.75
		rxoffs = 7.5
	end

	local formspec = "image["..lxoffs..",3.6;1,2;"..minetest.formspec_escape(skin:get_preview()).."]"
	if texture then
		formspec = formspec.."label["..rxoffs..",3.5;"..S("Raw texture")..":]"
		.."image["..rxoffs..",1;"..raw_size..";"..texture.."]"
	end
	if m_name ~= "" then
		formspec = formspec.."label["..cxoffs..",3.5;"..S("Name")..": "..minetest.formspec_escape(m_name).."]"
	end
	if m_author ~= "" then
		formspec = formspec.."label["..cxoffs..",3;"..S("Author")..": "..minetest.formspec_escape(m_author).."]"
	end
	if m_license ~= "" then
		formspec = formspec.."label["..cxoffs..",4;"..S("License")..": "..minetest.formspec_escape(m_license).."]"
	end
	if perplayer_formspec then
		formspec = formspec .. "label[8.4,"..(perplayer_formspec.formspec_y + 7.9)..";" .. MineStars.Translator("Trash:") .. "]"
		formspec = formspec .. "list[detached:trash;main;8.5,"..(perplayer_formspec.formspec_y + 8.3)..";1,1;]"
		formspec = formspec .. "background[8.5,"..(perplayer_formspec.formspec_y + 8.3)..";1,1;ui_single_slot.png]"
		formspec = formspec .. "label[8.3,"..(perplayer_formspec.formspec_y + 5.9)..";" .. MineStars.Translator("Offhand:") .. "]"
		formspec = formspec .. "list[current_player;offhand;8.5,"..(perplayer_formspec.formspec_y + 6.3)..";1,1;]"
		formspec = formspec .. "background[8.5,"..(perplayer_formspec.formspec_y + 6.3)..";1,1;ui_single_slot.png]"
		formspec = formspec .. "background[0,"..(perplayer_formspec.formspec_y + 6.3)..";8,4;ui_main_inventory.png]"
	end
	return formspec
end--]]

function skins.get_skin_selection_formspec(player, context, perplayer_formspec)
	context.skins_list = skins.get_skinlist_for_player(player:get_player_name())
	context.total_pages = 1
	local xoffs = 0
	local yoffs = 3
	local xspc = 1
	local yspc = 2
	local skinwidth = 1
	local skinheight = 2
	local xscale = 1 -- luacheck: ignore
	local btn_y = 8.15
	local drop_y = 5
	local btn_width = 1
	local droppos = 1
	local droplen = 6.25
	local btn_right = 7
	local maxdisp = 16
	local TOSUM = 0
	local NONUI = 1
	local UISUM = 0

	local ctrls_height = 0.5

	if type(perplayer_formspec) == "table" then -- it's being used under Unified Inventory
		xoffs =  0
		NONUI = 0
		UISUM = 1.7
		TOSUM = 1.3
		xspc =   ui.imgscale
		yspc =   ui.imgscale*2
		skinwidth =  ui.imgscale*0.9
		skinheight = ui.imgscale*1.9
		xscale = ui.imgscale
		btn_width = ui.imgscale
		droppos = xoffs + btn_width + 0.1
		droplen = ui.imgscale * 7 
		btn_right = droppos + droplen - 0.3

		if perplayer_formspec.pagecols == 4 then -- and we're in lite mode
			yoffs =  1
			maxdisp = 8
			drop_y = yoffs + skinheight + 0.1
		else
			yoffs =  1
			drop_y = yoffs + skinheight*2 + 0.2
		end

		btn_y = drop_y + 0.5
		

	end

	for i, skin in ipairs(context.skins_list ) do
		local page = math.floor((i-1) / maxdisp)+1
		skin:set_meta("inv_page", page)
		skin:set_meta("inv_page_index", (i-1)%maxdisp+1)
		context.total_pages = page
	end
	context.skins_page = context.skins_page or skins.get_player_skin(player):get_meta("inv_page") or 1
	context.dropdown_values = nil

	local page = context.skins_page
	local formspec = ""

	for i = (page-1)*maxdisp+1, page*maxdisp do
		local skin = context.skins_list[i]
		if not skin then
			break
		end

		local index_p = skin:get_meta("inv_page_index")
		local x = ((index_p-1) % 8) * xspc + xoffs
		local y
		if index_p > 8 then
			y = yoffs + yspc
		else
			y = yoffs
		end
		formspec = formspec..
			string.format("image_button[%f,%f;%f,%f;%s;skins_set$%i;]",
				x, y+TOSUM+0.3, skinwidth, skinheight,
				minetest.formspec_escape(skin:get_preview()), i)..
			"tooltip[skins_set$"..i..";"..minetest.formspec_escape(skin:get_meta_string("name")).."]"
	end

	if context.total_pages > 1 then
		local page_prev = page - 1
		local page_next = page + 1
		if page_prev < 1 then
			page_prev = context.total_pages
		end
		if page_next > context.total_pages then
			page_next = 1
		end
		local page_list = ""
		context.dropdown_values = {}
		for pg=1, context.total_pages do
			local pagename = S("Page").." "..pg.."/"..context.total_pages
			context.dropdown_values[pagename] = pg
			if pg > 1 then page_list = page_list.."," end
			page_list = page_list..pagename
		end
		formspec = formspec..
			string.format("button[%f,%f;%f,%f;skins_page$%i;<<]",
				xoffs, btn_y-0.5+UISUM, btn_width, ctrls_height, page_prev)..
			string.format("button[%f,%f;%f,%f;skins_page$%i;>>]",
				btn_right, btn_y-0.5+UISUM, btn_width, ctrls_height, page_next)..
			string.format("dropdown[%f,%f;%f,%f;skins_selpg;%s;%i]",
				droppos, drop_y+1.5+NONUI, droplen, ctrls_height, page_list, page)
	end
	--some addons
	if type(perplayer_formspec) == "table" then
		formspec = formspec .. "label[8.4,"..(perplayer_formspec.formspec_y + 7.9)..";" .. MineStars.Translator("Trash:") .. "]"
		formspec = formspec .. "list[detached:trash;main;8.5,"..(perplayer_formspec.formspec_y + 8.3)..";1,1;]"
		formspec = formspec .. "background[8.5,"..(perplayer_formspec.formspec_y + 8.3)..";1,1;ui_single_slot.png]"
		formspec = formspec .. "label[8.3,"..(perplayer_formspec.formspec_y + 6.3)..";" .. MineStars.Translator("Offhand:") .. "]"
		formspec = formspec .. "list[detached:"..player:get_player_name().."_offhand;offhand;8.5,"..(perplayer_formspec.formspec_y + 6.7)..";1,1;]"
		formspec = formspec .. "background[8.5,"..(perplayer_formspec.formspec_y + 6.7)..";1,1;ui_single_slot.png]"
		formspec = formspec .. "background[0,"..(perplayer_formspec.formspec_y + 6.3)..";8,4;ui_main_inventory.png]"
	end
	return formspec
end

function skins.on_skin_selection_receive_fields(player, context, fields)
	for field, _ in pairs(fields) do
		local current = string.split(field, "$", 2)
		if current[1] == "skins_set" then
			--sfinv.set_page(player, "sfinv:crafting")
			--sfinv.set_player_inventory_formspec(player)
			skins.set_player_skin(player, context.skins_list[tonumber(current[2])])
			unified_inventory.set_inventory_formspec(player, "skins")
			return 'set'
		elseif current[1] == "skins_page" then
			context.skins_page = tonumber(current[2])
			return 'page'
		end
	end
	if fields.skins_selpg then
		context.skins_page = tonumber(context.dropdown_values[fields.skins_selpg])
		return 'page'
	end
end
