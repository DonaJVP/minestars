-- Minetest 0.4 mod: player
-- See README.txt for licensing and other information.

-- Player animation blending
-- Note: This is currently broken due to a bug in Irrlicht, leave at 0
local animation_blend = 0

default.registered_player_models = { }

-- Local for speed.
local models = default.registered_player_models

function default.player_register_model(name, def)
	models[name] = def
end

-- Default player appearance
default.player_register_model("skinsdb_3d_armor_character_5.b3d", {
	animation_speed = 30,
	textures = {
		"character.png",
		"blank.png",
		"blank.png",
		"blank.png"
	},
	animations = {
		stand = {x=0, y=79},
		lay = {x=162, y=166},
		walk = {x=168, y=187},
		mine = {x=189, y=198},
		walk_mine = {x=200, y=219},
		sit = {x=81, y=160},
		--wave = {x = 192, y = 196, override_local = true},
		--point = {x = 196, y = 196, override_local = true},
		--freeze = {x = 205, y = 205, override_local = true},
		sneak = {x = 230, y = 290},
		sneakwalk = {x = 300, y = 340, override_local = true},
		sneakmine = {x = 560, y = 600, override_local = true},
		sneakwalkmine = {x = 350, y = 410, override_local = true},
		wave = {x = 430, y = 470},
		applause = {x = 480, y = 530},
		zombie = {x=1100,y=1130},
		useshield = {x=625,y=645,speed=15, override_local = true},
		useshieldrun = {x=670,y=710, override_local = true},
		useshieldmine = {x=950,y=990, override_local = true},
		useshieldrunmine = {x=1150,y=1190, override_local = true},
		useshieldsneak = {x=740,y=800, override_local = true},
		useshieldsneakmine = {x=1020,y=1060,override_local = true},
		useshieldsneakrun = {x=810,y=850, override_local = true},
		useshieldsneakrunmine = {x=870,y=910, override_local = true},
	},
})

-- Player stats and animations
local player_model = {}
local player_textures = {}
local player_anim = {}
local player_sneak = {}
default.player_attached = {}

function default.player_get_animation(player)
	local name = player:get_player_name()
	return {
		model = player_model[name],
		textures = player_textures[name],
		animation = player_anim[name],
	}
end

-- Called when a player's appearance needs to be updated
function default.player_set_model(player, model_name)
	local name = player:get_player_name()
	local model = models[model_name]
	if model then
		if player_model[name] == model_name then
			return
		end
		player:set_properties({
			mesh = model_name,
			textures = player_textures[name] or model.textures,
			visual = "mesh",
			visual_size = model.visual_size or {x=1, y=1},
		})
		default.player_set_animation(player, "stand")
	else
		player:set_properties({
			textures = { "player.png", "player_back.png", },
			visual = "upright_sprite",
		})
	end
	player_model[name] = {name = model_name, animation_speed = 30}
end

function default.player_set_textures(player, textures)
	local name = player:get_player_name()
	player_textures[name] = textures
	player:set_properties({textures = textures,})
end

function default.player_set_animation(player, anim_name, speed)
	--local t1 = core.get_us_time()
	local name = player:get_player_name()
	if player_anim[name] == anim_name then
		return
	end
	local model = player_model[name] and models[player_model[name].name]
	if not (model and model.animations[anim_name]) then
		return
	end
	local anim = model.animations[anim_name]
	player_anim[name] = anim_name
	player:set_animation(anim, speed or model.animation_speed, animation_blend)
	
	--print(string.format("elapsed time: %g ms", (core.get_us_time() - t1) / 1000))
end

local vector_z_sum = {}

-- Update appearance when the player joins
minetest.register_on_joinplayer(function(player)
	--core.after(0.9, function(player)
		default.player_attached[player:get_player_name()] = false
		default.player_set_model(player, "skinsdb_3d_armor_character_5.b3d")
		player:set_local_animation({x=0, y=79}, {x=168, y=187}, {x=189, y=198}, {x=200, y=219}, 30)

		player:hud_set_hotbar_image("gui_hotbar.png")
		player:hud_set_hotbar_selected_image("gui_hotbar_selected.png")
	--vector_z_sum[player:get_player_name()] = 0
	--end, player)
end)

minetest.register_on_leaveplayer(function(player)
	local name = player:get_player_name()
	player_model[name] = nil
	player_anim[name] = nil
	player_textures[name] = nil
end)

--PLAYER HEAD
--well, when the script sets a bone in other position  the clients that has 0.4.** versions has visual bugs (mesh does not accept animations!)
-- using boxed entities with the player head

-- Localize for better performance.
local player_set_animation = default.player_set_animation
local player_attached = default.player_attached
local pressed = {}

--[[
-- Check each player and apply animations
core.register_globalstep(function(dtime, _, controlmain)
	--print("dsjiofgihsrrrgyh", dtime, _, dump(controlmain))
	for name, controls in pairs(controlmain) do
		--print(controls, name)
		local model = player_model[name]
		if model and not player_attached[name] then
			local animation_speed_mod = model.animation_speed or 30
			
			
			Controls[name] = controls
			
			--Val
			
			local using_shield = false
			
			if not MineStars.OldClients[name] then
				if controls and controls.aux1 then
					animation_speed_mod = animation_speed_mod / 2
					if not pressed[name] then
						pressed[name] = true
						MineStars.Shield.SetStateOfPlayer(controls.player, true)
					end
				else
					pressed[name] = nil
					if MineStars.Shield.States[name] then
						MineStars.Shield.SetStateOfPlayer(controls.player, false)
					end
				end
			else
				if controls and controls.place then
					using_shield = true
					animation_speed_mod = animation_speed_mod / 2
					if not pressed[name] then
						pressed[name] = true
						MineStars.Shield.SetStateOfPlayer(controls.player, true)
						
					end
				else
					pressed[name] = nil
					if MineStars.Shield.States[name] then
						MineStars.Shield.SetStateOfPlayer(controls.player, false)
					end
				end
			end
			
			local got_animated = false
			
			--Determine where is player
			local player = controls.player
			
			-- Determine if the player is sneaking, and reduce animation speed if so
			if controls.sneak then
				animation_speed_mod = animation_speed_mod / 2
			end
			-- Apply animations based on what the player is doing
			if player:get_hp() == 0 then
				player_set_animation(player, "lay")
			elseif controls.up or controls.down or controls.left or controls.right then
				if MineStars then
					if controls.dig or (controls.place and using_shield == false) then
						--apply sneak
						if controls.sneak then
							if MineStars.Shield.States[name] then
								player_set_animation(player, "useshieldsneakrunmine", animation_speed_mod)
							else
								player_set_animation(player, "sneakwalkmine", animation_speed_mod)
							end
							got_animated = true
						else
							if MineStars.Shield.States[name] then
								player_set_animation(player, "useshieldrunmine", animation_speed_mod)
							else
								player_set_animation(player, "walk_mine", animation_speed_mod)
							end
							got_animated = true
						end
					else
						if controls.sneak then
							if MineStars.Shield.States[name] then
								player_set_animation(player, "useshieldsneakrun", animation_speed_mod)
							else
								player_set_animation(player, "sneakwalk", animation_speed_mod)
							end
							got_animated = true
						else
							if MineStars.Shield.States[name] then
								player_set_animation(player, "useshieldrun", animation_speed_mod)
							else
								player_set_animation(player, "walk", animation_speed_mod)
							end
							got_animated = true
						end
					end
				end
			elseif (controls.dig) or (controls.place and using_shield == false) then
				if controls.sneak then
					if MineStars and MineStars.Shield and MineStars.Shield.States[name] then
						player_set_animation(player, "useshieldsneakmine", animation_speed_mod)
					else
						player_set_animation(player, "sneakmine", animation_speed_mod)
					end
					got_animated = true
				else
					if MineStars and MineStars.Shield and MineStars.Shield.States[name] then
						player_set_animation(player, "useshieldmine", animation_speed_mod)
					else
						player_set_animation(player, "mine", animation_speed_mod)
					end
					got_animated = true
				end
			end
			
			if not got_animated then
				if controls.sneak then
					if MineStars and MineStars.Shield and MineStars.Shield.States[name] then
						player_set_animation(player, "useshieldsneak", animation_speed_mod)
					else
						player_set_animation(player, "sneak", animation_speed_mod)
					end
				else
					if MineStars and MineStars.Shield and MineStars.Shield.States[name] then
						player_set_animation(player, "useshield", animation_speed_mod)
					else
						player_set_animation(player, "stand", animation_speed_mod)
					end
				end
			end
			
			--Hacks for among almost all clients:
			if controls.sneak then
				if not MineStars.Shield.States[name] then
					controls.player:set_local_animation({x = 230, y = 290}, {x = 300, y = 340}, {x = 560, y = 600}, {x = 350, y = 410}, 15)
				else
					controls.player:set_local_animation({x=740,y=800}, {x=810,y=850}, {x=1020,y=1060}, {x=870,y=910}, 15)
				end
			else
				if not MineStars.Shield.States[name] then
					controls.player:set_local_animation({x=0, y=79}, {x=168, y=187}, {x=189, y=198}, {x=200, y=219}, 30)
				else
					controls.player:set_local_animation({x=625,y=645}, {x=670,y=710}, {x=950,y=990}, {x=1150,y=1190}, 30)
				end
			end
			
		end
	end
end)--]]

--MULT = 0
--MILT = 0

local already_set = {}

--[[
stand = {x=0, y=79},
		lay = {x=162, y=166},
		walk = {x=168, y=187},
		mine = {x=189, y=198},
		walk_mine = {x=200, y=219},
		sit = {x=81, y=160},
		--wave = {x = 192, y = 196, override_local = true},
		--point = {x = 196, y = 196, override_local = true},
		--freeze = {x = 205, y = 205, override_local = true},
		sneak = {x = 230, y = 290},
		sneakwalk = {x = 300, y = 340, override_local = true},
		sneakmine = {x = 560, y = 600, override_local = true},
		sneakwalkmine = {x = 350, y = 410, override_local = true},
		wave = {x = 430, y = 470},
		applause = {x = 480, y = 530},
		zombie = {x=1100,y=1130},
		useshield = {x=625,y=645,speed=15, override_local = true},
		useshieldrun = {x=670,y=710, override_local = true},
		useshieldmine = {x=950,y=990, override_local = true},
		useshieldrunmine = {x=1150,y=1190, override_local = true},
		useshieldsneak = {x=740,y=800, override_local = true},
		useshieldsneakmine = {x=1020,y=1060,override_local = true},
		useshieldsneakrun = {x=810,y=850, override_local = true},
		useshieldsneakrunmine = {x=870,y=910, override_local = true},
--]]

local aset = {}
local function set_local_anim(player, anim)
	local n = player:get_player_name()
	if aset[n] == anim then return end
	aset[n] = anim
	if anim == 0 then --only sneak anim
		--if aset[n] ~= 0 then
			player:set_local_animation({x = 230, y = 290}, {x = 300, y = 340}, {x = 560, y = 600}, {x = 350, y = 410}, 15)
		--	aset[n] = 0
		--end
	elseif anim == 1 then --only shield
		--if aset[n] ~= 1 then
			player:set_local_animation({x=625,y=645}, {x=670,y=710}, {x=950,y=990}, {x=1150,y=1190}, 15)
		--	aset[n] = 1
		--end
	elseif anim == 2 then --standard anim
		--if aset[n] ~= 2 then
			player:set_local_animation({x=0, y=79}, {x=168, y=187}, {x=189, y=198}, {x=200, y=219}, 30)
		--	aset[n] = 2
		--end
	elseif anim == 3 then --both
		--if aset[n] ~= 3 then
			player:set_local_animation({x=740,y=800}, {x=810,y=850}, {x=1020,y=1060}, {x=870,y=910}, 15)
		--	aset[n] = 3
		--end
	end
end

default.LastPitch = {}

--vector_z_sum[player:get_player_name()] +
function core.head_move_func(player, pitch)
	default.LastPitch[player:get_player_name()] = pitch
	if not MineStars.Emotes.PlayersDoing[Name(player)] then
		MineStars.HeadsObjects[player:get_player_name()]:set_bone_position("Head", {x = 0, y = 6.65 + (Sneaking[player:get_player_name()] and -1.2 or 0), z = ((Sneaking[player:get_player_name()] and 2.35) or 0)}, {x = -pitch, y = -184, z = 1})
	else
		MineStars.HeadsObjects[player:get_player_name()]:set_bone_position("Head", {x = 0, y = 6.65 + MineStars.Emotes.Registered[MineStars.Emotes.PlayersDoing[player:get_player_name()]].SumY, z = MineStars.Emotes.Registered[MineStars.Emotes.PlayersDoing[player:get_player_name()]].SumZ}, {x = -pitch, y = -184, z = 1})
	end
	--print("REGISTER")
end
--[[
--s_env.cpp & serverpackethandler.cpp
function core.func_animate_player(player, clc, walk, aux, sneak, click)
	--Front Animations
	local name = player:get_player_name()
	--print(pitch, player:get_look_vertical())
	--print(clc, walk, aux, sneak, dig, place)
	--if MineStars.HeadsObjects[name] then
	--	MineStars.HeadsObjects[name]:set_bone_position("Head", {x = MULT, y = 6.65, z = MILT}, {x = -pitch, y = -184, z = 0})
	--end
	--Shield
	
	local walk = (bits & (0x1 << 0)) or (bits & (0x1 << 1)) or (bits & (0x1 << 2)) or (bits & (0x1 << 3))
	local aux = (bits & (0x1 << 5))
	local click = (bits & (0x1 << 7)) or (bits & (0x1 << 8)
	
	
	
	
	if aux and not already_set[name] then
		MineStars.Shield.SetStateOfPlayer(player, true)
		already_set[name] = true
		--print("set")
	elseif not aux then
		MineStars.Shield.SetStateOfPlayer(player, false)
		already_set[name] = false
		--print("not set")
	end
	--Player
	if walk then
		if click then
			if sneak then
				if MineStars.Shield.States[name] then
					player_set_animation(player, "useshieldsneakrunmine", 15)
				else
					player_set_animation(player, "sneakwalkmine", 15)
				end
			else
				if MineStars.Shield.States[name] then
					player_set_animation(player, "useshieldrunmine", 30)
				else
					player_set_animation(player, "walk_mine", 30)
				end
			end
		else
			if sneak then
				if MineStars.Shield.States[name] then
					player_set_animation(player, "useshieldsneakrun", 15)
				else
					player_set_animation(player, "sneakwalk", 15)
				end
			else
				if MineStars.Shield.States[name] then
					player_set_animation(player, "useshieldrun", 30)
				else
					player_set_animation(player, "walk", 30)
				end
			end
		end
	elseif click then
		if sneak then
			if MineStars.Shield.States[name] then
				player_set_animation(player, "useshieldsneakmine", 15)
			else
				player_set_animation(player, "sneakmine", 15)
			end
		else
			if MineStars.Shield.States[name] then
				player_set_animation(player, "useshieldmine", 30)
			else
				player_set_animation(player, "mine", 30)
			end
		end
	elseif sneak then
		if MineStars.Shield.States[name] then
			player_set_animation(player, "useshieldsneak", 15)
		else
			player_set_animation(player, "sneak", 15)
		end
	else
		player_set_animation(player, "stand", 15)
	end
	--Local Animations
	if sneak then
		if MineStars.Shield.States[name] then
			set_local_anim(player, 3)
		else
			set_local_anim(player, 0)
		end
	else
		if MineStars.Shield.States[name] then
			set_local_anim(player, 1)
		else
			set_local_anim(player, 2)
		end
	end
end
--]]

local indexed__ = {}

function core.func_animate_player(player, walk, aux, sneak, click, pitch)
	--do return end
	default.LastPitch[player:get_player_name()] = pitch
	
	player:set_player_sum(0,0)
	--OnChangeSkin(player, pitch)
	--local t1 = core.get_us_time()
	if MineStars.Emotes.PlayersDoing[Name(player)] then
		MineStars.Emotes.PlayersDoing[Name(player)] = nil
		MineStars.HeadsObjects[player:get_player_name()]:set_bone_position("Head", {x = 0, y = 6.65, z = 0}, vector.new(-pitch, -184, 0))
	end
	local name = player:get_player_name()
	if sneak == true and not Sneaking[name] then
		MineStars.HeadsObjects[player:get_player_name()]:set_bone_position("Head", {x = 0, y = 5.45, z = 2.35}, vector.new(-pitch, -184, 0))
		--[[player:set_properties({
			eye_height = 1.40,
		})--]]
	elseif not sneak and Sneaking[name] then
		MineStars.HeadsObjects[player:get_player_name()]:set_bone_position("Head", {x = 0, y = 6.65, z = 0}, vector.new(-pitch, -184, 0))
		--[[player:set_properties({
			eye_height = 1.625,
		})--]]
	end
	
	if aux then
		if MineStars.Food.GetLevel(player) > 4 then
			MineStars.Food.IncrementSpeed(player);
		else
			MineStars.Food.DecrementSpeed(player);
		end
	else
		MineStars.Food.DecrementSpeed(player);
	end
	
	Sneaking[name] = sneak
	local shield_state = MineStars.Shield.States[Name(player)]
	if sneak and not already_set[name] then
		shield_state = MineStars.Shield.SetStateOfPlayer(player, true)
		already_set[name] = true
	elseif not sneak then
		shield_state = MineStars.Shield.SetStateOfPlayer(player, false)
		already_set[name] = false
	end
	
	local animation, speed = nil, 30
	
	if walk then
		if sneak then
			animation = shield_state and (click and "useshieldsneakrunmine" or "useshieldsneakrun") or (click and "sneakwalkmine" or "sneakwalk")
			speed = 15
		else
			animation = shield_state and (click and "useshieldrunmine" or "useshieldrun") or (click and "walk_mine" or "walk")
			speed = 30
		end
	elseif click then
		animation = shield_state and (sneak and "useshieldsneakmine" or "useshieldmine") or (sneak and "sneakmine" or "mine")
		speed = sneak and 15 or 30
	elseif sneak then
		animation = shield_state and "useshieldsneak" or "sneak"
		speed = 15
	else
		animation = "stand" --shield_state and "useshield" or 
		speed = 15
	end
	
	--if walk then
	--	vector_z_sum[name] = 0.5
	--else
	--	vector_z_sum[name] = 0
	--end
	
	--print(player_set_animation, set_local_anim)
	
	player_set_animation(player, animation, speed)
	
	local local_anim = (sneak and (shield_state and 3 or 0)) or (shield_state and 1 or 2)
	
	set_local_anim(player, local_anim)
	
	--print(string.format("elapsed time: %g ms", (core.get_us_time() - t1) / 1000))
	--print(local_anim, aux)
end

core.register_on_dieplayer(function(player, reason)
	--player:set_animation({x=162, y=166}, 30) --due to bones incompatibility
	player:set_properties({
		visual_size = vector.new(0,0,0)
	})
end)

core.register_on_respawnplayer(function(player)
	player:set_properties({
		visual_size = vector.new(1,1,1)
	})
end)
