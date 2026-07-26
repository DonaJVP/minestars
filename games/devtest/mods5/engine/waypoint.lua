MineStars.Waypoint = {
	Cache = {},
}

core.register_entity("engine:waypoint",{
	initial_properties = {
		hp_max = 20,
		collide_with_objects = true,
		visual = "sprite",
		visual_size = {x = 1, y = 1, z = 1},
		collisionbox = {0.3, 0.3, 0.3, -0.3, -0.3, -0.3},
		selectionbox = {0.3, 0.3, 0.3, -0.3, -0.3, -0.3},
		physical = true,
		is_visible = true,
		pointable = true,
		textures = {"sprite_552.png"},
		static_save = true,
	},
	waypoint = true,
	label = "",
	owner = "",
	labelcolor = "#FFFFFF",
	on_activate = function(self, st)
		local splitted = st:split(";")
		--print(dump(splitted))
		self.owner = splitted[3] or (self.owner and self.owner ~= "" and self.owner) or ""
		self.label = splitted[1] or (self.label and self.label ~= "" and self.label) or "Not set yet."
		self.labelcolor = splitted[2] or "#FFFFFF"
		self.object:set_properties({nametag = core.colorize(self.labelcolor, self.label)})
	end,
	get_staticdata = function(self)
		local labelcolor = self.labelcolor or "#FFFFFF"
		if self.label then
			return self.label..";"..labelcolor..";"..self.owner
		end
	end,
})

local function return_formspec(nameres, subn)
	return "size[2,1]" ..
	"field[0.1,0.2;2.4,1;field;"..(subn or "Waypoint Label")..";"..nameres.."]" ..
	"image_button_exit[-0.18,0.7;2.43,0.5;blank.png;btnsave;"..MineStars.Translator("Save").."]" ..
	"image_button_exit[-0.18,1.05;2.43,0.5;blank.png;btn_quit;"..MineStars.Translator("Quit & Don't save").."]"
end

core.register_tool("engine:waypoint", {
	description = MineStars.Translator"Summon or modify a waypoint",
	inventory_image = "sprite_552.png",
	on_use = function(itemstack, user, pointed_thing)
		if pointed_thing.ref then
			local ent = pointed_thing.ref:get_luaentity()
			--print(dump(ent))
			if ent then
				if ent.waypoint and ent.owner == Name(user) then
					core.show_formspec(Name(user), "waypoint:main", return_formspec((ent.label and ent.label ~= "" and ent.label) or "Not set yet."))
					MineStars.Waypoint.Cache[Name(user)] = pointed_thing.ref
				end
			end
		else
			if pointed_thing.above then
				local pos = CheckPos(pointed_thing.above)
				local obj = core.add_entity(pos, "engine:waypoint")
				MineStars.Waypoint.Cache[Name(user)] = obj
				local ent = obj:get_luaentity()
				if ent then
					ent.label = "No set yet."
					ent.labelcolor = "#FFFFFF"
					ent.owner = Name(user)
					obj:set_properties({nametag = core.colorize("#FFFFFF", "No set yet.\nSet me!"), pointable = true})
				end
			end
		end
		return itemstack
	end
})

core.register_tool("engine:waypoint_color", {
	description = MineStars.Translator"Modify a waypoint color",
	inventory_image = "sprite_553.png",
	on_use = function(itemstack, user, pointed_thing)
		if pointed_thing.ref then
			local ent = pointed_thing.ref:get_luaentity()
			if ent then
				if ent.waypoint and ent.owner == Name(user) then
					core.show_formspec(Name(user), "waypoint:color", return_formspec(((ent.labelcolor ~= "" and ent.labelcolor) or "#FFFFFF"), "Color editor"))
					MineStars.Waypoint.Cache[Name(user)] = pointed_thing.ref
				end
			end
		end
		return itemstack
	end
})

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "waypoint:color" then
		local obj = MineStars.Waypoint.Cache[Name(player)]
		if obj and not fields.btn_quit then
			if fields.field and fields.field ~= "" then
				local ent = obj:get_luaentity()
				if ent then
					if ent.waypoint then
						ent.labelcolor = fields.field
						obj:set_properties({
							nametag = core.colorize(fields.field, ent.label)
						})
						ent.owner = Name(player)
					end
				end
			end
		end
	elseif formname == "waypoint:main" then
		local obj = MineStars.Waypoint.Cache[Name(player)]
		if obj and not fields.btn_quit then
			if fields.field and fields.field ~= "" then
				local ent = obj:get_luaentity()
				if ent then
					if ent.waypoint then
						ent.label = fields.field
						obj:set_properties({
							nametag = core.colorize(ent.labelcolor or "#FFFFFF", fields.field)
						})
						ent.owner = Name(player)
					end
				end
			end
		end
	end
end)






















