local S = minetest.get_translator("skinsdb")

unified_inventory.register_page("skins", {
	get_formspec = function(player, perplayer_formspec)
		local skin = skins.get_player_skin(player)
		local boffs = 0.75

		local formspec = skins.get_skin_info_formspec(skin, perplayer_formspec).."button["..boffs..",6.5;6.5,.5;skins_page;"..S("Change").."]"
		return {formspec=formspec}
	end,
})

local function get_formspec(player, perplayer_formspec)
	local context = skins.get_formspec_context(player)
	local formspec = skins.get_skin_selection_formspec(player, context, perplayer_formspec)
	return formspec
end

unified_inventory.register_page("skins_page", {
	get_formspec = function(player, perplayer_formspec)
		return {formspec=get_formspec(player, perplayer_formspec)}
	end
})

-- click button handlers
minetest.register_on_player_receive_fields(function(player, formname, fields)
	if fields.skins then
		unified_inventory.set_inventory_formspec(player, "skins")
		return
	end

	if formname ~= "" then
		return
	end


	local context = skins.get_formspec_context(player)
	local action = skins.on_skin_selection_receive_fields(player, context, fields)
	if action == 'set' then
		unified_inventory.set_inventory_formspec(player, "skins")
	elseif action == 'page' then
		unified_inventory.set_inventory_formspec(player, "skins_page")
	end
end)
