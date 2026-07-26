----
-- Compatibility with 0.4 series
----

MineStars.HeadsObjects = {}

local function get_boxes_for_entity(raw_texture)
	local front = "(([combine:8x8:-8,-8="..raw_texture..")^[resize:8x8)";
	local left = "(([combine:8x8:0,-8="..raw_texture..")^[resize:8x8)";
	local right = "(([combine:8x8:-16,-8="..raw_texture..")^[resize:8x8)";
	local up = "(([combine:8x8:-8,0="..raw_texture..")^[resize:8x8)";
	local back = "(([combine:8x8:-24,-8="..raw_texture..")^[resize:8x8)";
	local down = "(([combine:8x8:-16,0="..raw_texture..")^[resize:8x8)";
	return {
		up,
		down,
		right,
		left,
		back,
		front
	}
end

core.register_entity("engine:head_entity", {
	initial_properties = {
		visual = "mesh",
		mesh = "head.b3d", --HACK
		visual_size = vector.new(1,1,1),
		textures = {
			"character.png",
			"blank.png",
			"blank.png",
			"blank.png",
		},
		physical = false,
		collisionbox = {0,0,0,0,0,0},
		pointable = false,
		is_visible = true,
		static_save = false,
	},
	head = true,
	player = "",
	on_detach = function(self, parent)
		self.object:remove()
	end
})

--ANIMATION WITH ENTIRE BODY AAAHHAHHAHHAHRARHARW

local function update_player_visuals(player, p)
	--Remove current object (bug)
	if MineStars.HeadsObjects[player:get_player_name()] then
		MineStars.HeadsObjects[player:get_player_name()]:set_detach()
		MineStars.HeadsObjects[player:get_player_name()]:remove()
		MineStars.HeadsObjects[player:get_player_name()] = nil
	end
	--Make is head not visible, it will be replaced with new
	local t__ = player:get_properties().textures; --no
	
	local skin = skins.get_player_skin(player)
	local textures_of_player = {
		[1] = skin:get_texture(),
		[2] = "blank.png",
		[3] = armor:get_textures(player) and armor:get_textures(player).armor or "blank.png",
		[4] = t__[4],
	}
	
	local png1 = textures_of_player[1]; --1.0 [IGNORED]
	local png1_ = textures_of_player[2]; --1.8 [USED]
	local armor = textures_of_player[3]; --armor
	--if png1_ ~= "blank.png"
	
	--Make the visuals
	local visuals = {}--get_boxes_for_entity(png1, armor)
	
	--make png1 transparent some parts
	png1 = png1.."^[mask:onlybody.png"; --make the body seeable, but head not
	armor = (textures_of_player[4] ~= "blank.png") and armor.."^[mask:onlybodyarmor.png"; --this too
	
	player:set_properties({
		textures = {
			png1,
			png1_,
			armor or "blank.png",
			textures_of_player[4],
		}
	});
	
	local obj = core.add_entity(player:get_pos(), "engine:head_entity");
	if obj then
		MineStars.HeadsObjects[player:get_player_name()] = obj;
		obj:set_attach(player, "", vector.new(0,6.65,0), vector.new(0,0,0)) --23.5, -6.65-16
		obj:set_properties({
			textures = {
				textures_of_player[1].."^[mask:overlayforhead.png",
				png1_,
				--textures_of_player[3].."^[mask:overlayforhead.png",
				(textures_of_player[3] ~= "blank.png") and textures_of_player[3].."^[mask:onlybodyarmorhelmet.png" or "blank.png",
				--"blank.png"
				"blank.png",
			}
		})
		
		--(p or 0)
		local num = p or 0
		if num ~= 0 then
			num = -num
		end
		--print(num)
		obj:set_bone_position("Head", vector.new(0,6.65,0), vector.new(num,-184,0))
		return true
	else
		return false
	end
end

function OnChangeSkin(player, p)
	return update_player_visuals(player, p)
end

OnChangeArmorStructure = OnChangeSkin

core.register_on_joinplayer(function(player)
	update_player_visuals(player)
end)
--[[
core.register_on_leaveplayer(function(player)
	if MineStars.HeadsObjects[player:get_player_name()] then
		MineStars.HeadsObjects[player:get_player_name()]:remove()
		MineStars.HeadsObjects[player:get_player_name()] = nil
	end
end)

core.register_on_dieplayer(function(player)
	--Reset head
	if MineStars.HeadsObjects[player:get_player_name()] then
		MineStars.HeadsObjects[player:get_player_name()]:remove()
		MineStars.HeadsObjects[player:get_player_name()] = nil
	end
end)--]]

--Object is always attached to the player, just make it dissapear when the player dies
local function update_visuals(player)
	local obj = MineStars.HeadsObjects[Name(player)]
	if obj then
		obj:set_attach(player, "", vector.new(0,6.65,0), vector.new())
		local skin = skins.get_player_skin(player)
		local textures_of_player = {
			[1] = skin:get_texture(),
			[2] = "blank.png",
			[3] = armor:get_textures(player) and armor:get_textures(player).armor or "blank.png",
			[4] = "blank.png",
		}
		--Proccess
		obj:set_properties({
			textures = {
				textures_of_player[1].."^[mask:overlayforhead.png",
				"blank.png",
				(textures_of_player[3] ~= "blank.png") and textures_of_player[3].."^[mask:onlybodyarmorhelmet.png" or "blank.png",
				"blank.png",
			}
		})
	else
		update_player_visuals(player)
	end
end

core.register_on_respawnplayer(function(player)
	--print("respawn")
	core.after(0.5, update_visuals, player)
end)

armor:register_on_update(function(player)
	core.after(0.3, update_visuals, player)
end)

MineStars.UpdateHead = update_visuals

core.func_on_teleport = function(player)
	--error()
	--core.log("error", "Got here")
	core.after(0.3, OnChangeSkin, player) --Some bugs at engine
end






































--[[
----
-- Compatibility with 0.4 series
----

MineStars.HeadsObjects = {}

local function get_boxes_for_entity(raw_texture)
	local front = "(([combine:8x8:-8,-8="..raw_texture..")^[resize:8x8)";
	local left = "(([combine:8x8:0,-8="..raw_texture..")^[resize:8x8)";
	local right = "(([combine:8x8:-16,-8="..raw_texture..")^[resize:8x8)";
	local up = "(([combine:8x8:-8,0="..raw_texture..")^[resize:8x8)";
	local back = "(([combine:8x8:-24,-8="..raw_texture..")^[resize:8x8)";
	local down = "(([combine:8x8:-16,0="..raw_texture..")^[resize:8x8)";
	return {
		up,
		down,
		right,
		left,
		back,
		front
	}
end

core.register_entity("engine:head_entity", {
	initial_properties = {
		visual = "cube",
		visual_size = vector.new(0.4,0.4,0.4),
		textures = get_boxes_for_entity("character.png"), --default
		physical = false,
		collisionbox = {0,0,0,0,0,0},
		pointable = false,
		is_visible = true,
	},
	head = true,
	player = "",
	
})

--ANIMATION WITH ENTIRE BODY AAAHHAHHAHHAHRARHARW

local function update_player_visuals(player)
	--Make is head not visible, it will be replaced with new
	local textures_of_player = player:get_properties().textures;
	local png1 = textures_of_player[1]; --1.0 [IGNORED]
	local png1_ = textures_of_player[2]; --1.8 [USED]
	local armor = textures_of_player[3]; --armor
	--if png1_ ~= "blank.png"
	
	--Make the visuals
	local visuals = get_boxes_for_entity(png1, armor)
	
	--make png1 transparent some parts
	png1 = png1.."^[mask:onlybody.png"; --make the body seeable, but head not
	armor = armor.."^[mask:onlybody.png"; --this too
	
	player:set_properties({
		textures = {
			png1,
			png1_,
			armor,
			textures_of_player[4],
		}
	});
	
	local obj = core.add_entity(player:get_pos(), "engine:head_entity");
	if obj then
		MineStars.HeadsObjects[player:get_player_name()] = obj;
		obj:set_attach(player, "Head", vector.new(0,0,0))
		obj:set_properties({
			textures = visuals
		})
	end
end

function OnChangeSkin(player)
	core.after(0.3, update_player_visuals, player)
end

OnChangeArmorStructure = OnChangeSkin

core.register_on_joinplayer(function(player)
	core.after(0.6, update_player_visuals, player)
end)



















--]]
