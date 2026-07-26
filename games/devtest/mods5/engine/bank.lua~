MineStars.Bank = {
	CachePos = {},
}

local S = MineStars.Translator or function(...) return table.concat({...}, "") end

--[[
	"formspec_version[6]" ..
	"size[14,14.4]" ..
	"box[0,0;14,0.6;#62AA3F]" ..
	"label[0.1,0.3;Shop config]" ..
	"button_exit[0.2,13.5;13.6,0.7;exit;Save & Exit]" ..
	"list[pos;main;0.2,1.2;4,1;0]" ..
	"field[0.2,3.4;1.7,0.6;itm1_price;1° Item $$;0]" ..
	"field[0.2,4.5;1.7,0.6;itm2_price;2° Item $$;0]" ..
	"field[0.2,5.6;1.7,0.6;itm3_price;3° Item $$;0]" ..
	"field[0.2,6.7;1.7,0.6;itm4_price;4° Item $$;0]" ..
	"label[1.5,0.9;Items to show]" ..
	"label[0.6,2.5;1°]" ..
	"label[1.8,2.5;2°]" ..
	"label[3.05,2.5;3°]" ..
	"label[4.3,2.5;4°]" ..
	"list[current_player;main;2.2,3;8,4;0]" ..
	"list[current_player;main;2.1,8.6;8,4;0]" ..
	"label[6.2,8.3;Inventory]" ..
	"label[6.3,2.7;Storage]"
	
	"formspec_version[6]" ..
	"size[8.3,4.2]" ..
	"box[0,0;5.6,0.6;#62AA3F]" ..
	"label[0.1,0.3;Shop]" ..
	"image_button[0.3,1;1.7,1.7;default_tool_steelsword.png;itm1;;false;true]" ..
	"image_button[2.3,1;1.7,1.7;default_tool_mesesword.png;;;false;true]" ..
	"image_button[4.3,1;1.7,1.7;default_tool_diamondsword.png;;;false;true]" ..
	"image_button[6.3,1;1.7,1.7;default_tool_bronzesword.png;;;false;true]" ..
	"label[1,3;$1]" ..
	"label[2.95,3;$2]" ..
	"label[4.93,3;$3]" ..
	"label[6.9,3;$4]" ..
	"button_exit[0.3,3.4;7.7,0.7;exit;Exit]" ..
	"box[5.59,0;2.8,0.6;#22AB3F]" ..
	"label[5.7,0.3;B: $$$]"
--]]

local ShopTableFormspec = function(img1,img2,img3,img4,ia1,ia2,ia3,ia4,amount)
	return "size[6.25,2.89]" ..
	"box[-0.3,-0.32;4.48,0.52;#62AA3F]" ..
	"label[-0.22,-0.32;Shop]" ..
	"label[-0.06,2.02;"..(ia1 or 0).."]" ..
	"label[1.54,2.02;"..(ia2 or 0).."]" ..
	"label[3.14,2.02;"..(ia3 or 0).."]" ..
	"label[4.74,2.02;"..(ia4 or 0).."]" ..
	"image_button[-0.06,0.54;1.57,1.61;"..(img1 or "unavailable_item.png")..";itm1;;false;true]" ..
	"image_button[1.54,0.54;1.57,1.61;"..(img2 or "unavailable_item.png")..";itm2;;false;true]" ..
	"image_button[3.14,0.54;1.57,1.61;"..(img3 or "unavailable_item.png")..";itm3;;false;true]" ..
	"image_button[4.74,0.54;1.57,1.61;"..(img4 or "unavailable_item.png")..";itm4;;false;true]" ..
	"image_button_exit[-0.06,2.62;6.37,0.74;blank.png;exit;Exit]" ..
	"box[4.17,-0.32;2.24,0.52;#22AB3F]" ..
	"label[4.26,-0.32;B: $"..(amount or 0).."]"
end

local ShopConfigFormspec = function(p1,p2,p3,p4,pos)
	pos=pos.x .. "," .. pos.y .. "," .. pos.z
	return "size[10.81,11.73]" ..
	"box[-0.3,-0.32;11.2,0.52;#62AA3F]" ..
	"label[-0.22,-0.32;Shop config]" ..
	"image_button_exit[-0.14,11.38;11.09,0.74;blank.png;exit;Save & Exit]" ..
	"list[nodemeta:"..pos..";showarea;-0.14,0.72;4.0,1.0;0]" ..
	"field[0.16,2.91;1.56,0.69;itm1_price;1° Item $$;"..p1.."]" ..
	"field[0.16,3.86;1.56,0.69;itm2_price;2° Item $$;"..p2.."]" ..
	"field[0.16,4.81;1.56,0.69;itm3_price;3° Item $$;"..p3.."]" ..
	"field[0.16,5.77;1.56,0.69;itm4_price;4° Item $$;"..p4.."]" ..
	"label[0.9,0.2;Items to show]" ..
	"label[0.18,1.58;1°]" ..
	"label[1.14,1.58;2°]" ..
	"label[2.14,1.58;3°]" ..
	"label[3.14,1.58;4°]" ..
	"list[nodemeta:"..pos..";main;1.38,2.28;8.0,1.0;0]" ..
	"list[nodemeta:"..pos..";main;1.38,3.36;8.0,1.0;8]" ..
	"list[nodemeta:"..pos..";main;1.38,4.44;8.0,1.0;16]" ..
	"list[nodemeta:"..pos..";main;1.38,5.53;8.0,1.0;24]" ..
	"list[current_player;main;1.38,7.13;8.0,1.0;0]" ..
	"list[current_player;main;1.38,8.21;8.0,1.0;8]" ..
	"list[current_player;main;1.38,9.3;8.0,1.0;16]" ..
	"list[current_player;main;1.38,10.38;8.0,1.0;24]" ..
	"label[4.66,6.61;Inventory]" ..
	"label[4.74,1.76;Storage]"
end
---->
--Data>
---->

--Fast access
local fastaccess = {}

function MineStars.Bank.GetValue(__obj)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local val = fastaccess[Name(__obj)] or meta:get_int("money")
			if (val == 0) or not val then
				val = 0
			end
			fastaccess[Name(__obj)] = val
			return val
		end
	end
end
function MineStars.Bank.RmvValue(__obj, value)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local val = fastaccess[Name(__obj)] or meta:get_int("money")
			if (val == 0) or not val then
				val = 0
			end
			val = val - value
			val = AlwaysReturnPositiveInteger(val)
			fastaccess[Name(__obj)] = val
			meta:set_int("money", val)
			return true
		end
	end
end
function MineStars.Bank.AddValue(__obj, value)
	local player = Player(__obj)
	if player then
		local meta = player:get_meta()
		if meta then
			local val = fastaccess[Name(__obj)] or meta:get_int("money")
			if (val == 0) or not val then
				val = 0
			end
			val = val + value
			val = AlwaysReturnPositiveInteger(val)
			fastaccess[Name(__obj)] = val
			meta:set_int("money", val)
			return true
		end
	end
end
function MineStars.Bank.ShowToPlayerConfigShop(player, pos)
	local n = Name(player)
	if n and pos then
		MineStars.Bank.CachePos[n] = pos
		local meta = core.get_meta(pos)
		local data = core.deserialize(meta:get_string("prices")) or {}
		local enu0 = data[1] or 0
		local enu1 = data[2] or 0
		local enu2 = data[3] or 0
		local enu3 = data[4] or 0
		core.show_formspec(n, "bank:cfgshop", ShopConfigFormspec(enu0,enu1,enu2,enu3,pos))
	end
end
function MineStars.Bank.ShowToPlayerShop(player, pos)
	local n = Name(player)
	if n and pos then
		MineStars.Bank.CachePos[n] = pos
		local meta = core.get_meta(pos)
		local data1 = core.deserialize(meta:get_string("prices")) or {}
		local enu0 = data1[1] or 0
		local enu1 = data1[2] or 0
		local enu2 = data1[3] or 0
		local enu3 = data1[4] or 0
		local invdata = meta:get_inventory()
		local show_area_t = invdata:get_list("showarea")
		local itm0 = show_area_t[1]:get_name()
		local itm1 = show_area_t[2]:get_name()
		local itm2 = show_area_t[3]:get_name()
		local itm3 = show_area_t[4]:get_name()
		--Proceed with images
		local img0 = "unavailable_item.png"
		local img1 = "unavailable_item.png"
		local img2 = "unavailable_item.png"
		local img3 = "unavailable_item.png"
		if itm0 ~= "" then
			img0 = core.registered_items[itm0].inventory_image or core.registered_items[itm0].tiles[1] or core.registered_items[itm0].wield_image
		end
		if itm1 ~= "" then
			img1 = core.registered_items[itm1].inventory_image or core.registered_items[itm1].tiles[1] or core.registered_items[itm1].wield_image
		end
		if itm2 ~= "" then
			img2 = core.registered_items[itm2].inventory_image or core.registered_items[itm2].tiles[1] or core.registered_items[itm2].wield_image
		end
		if itm3 ~= "" then
			img3 = core.registered_items[itm3].inventory_image or core.registered_items[itm3].tiles[1] or core.registered_items[itm3].wield_image
		end
		--Proceed with prices
		local prc0 = "$0 x 0"
		local prc1 = "$0 x 0"
		local prc2 = "$0 x 0"
		local prc3 = "$0 x 0"
		if itm0 ~= "" then
			prc0 = "$"..enu0.." x "..show_area_t[1]:get_count()
		end
		if itm1 ~= "" then
			prc1 = "$"..enu1.." x "..show_area_t[2]:get_count()
		end
		if itm2 ~= "" then
			prc2 = "$"..enu2.." x "..show_area_t[3]:get_count()
		end
		if itm3 ~= "" then
			prc3 = "$"..enu3.." x "..show_area_t[4]:get_count()
		end
		core.show_formspec(n, "bank:shop", ShopTableFormspec(img0,img1,img2,img3,prc0,prc1,prc2,prc3,MineStars.Bank.GetValue(n)))
	end
end
function MineStars.Bank.AcquireItem(player, item_nu)
	local n = Name(player)
	if n then
		local pos = MineStars.Bank.CachePos[n]
		local meta = core.get_meta(pos)
		if meta then
			local invdata = meta:get_inventory()
			local show_area_t = invdata:get_list("showarea")
			local data1 = core.deserialize(meta:get_string("prices")) or {}
			--selection
			local itm = show_area_t[item_nu]
			local prc = data1[item_nu] or 0
			--core.chat_send_all(itm:get_name() ~= "" and itm:get_name() or "null", prc or 0)
			if itm:get_name() == "" then
				core.chat_send_player(Name(player), core.colorize("#FF8E8E", "✗"..S("Invalid item!")))
				return
			end
			--Process
			local moneyamount = MineStars.Bank.GetValue(player)
			if moneyamount >= prc then
				if invdata:contains_item("main", itm) then
					--invdata:remove_item("main", itm)
					if not (meta:get_int("creative_shop") > 0) then
						invdata:remove_item("main", itm)
					end
					MineStars.Bank.QueuePayTo(player, meta:get_string("owner"), prc, false, "Sold item for @1")
					if not Player(meta:get_string("owner")) then
						core.chat_send_player(Name(player), core.colorize("lightgreen", "✔ "..S("You got @1", itm:get_short_description().." x "..itm:get_count().." of $"..prc)))
					end
					player:get_inventory():add_item("main", itm)
				else
					core.chat_send_player(Name(player), core.colorize("#FF8E8E", "✗"..S("Could not buy @1", itm:get_short_description())))
				end
			else
				core.chat_send_player(Name(player), core.colorize("#FF8E8E", "✗"..S("Could not buy @1, no enough money!", itm:get_short_description())))
			end
		end
	end
end

---->
--Craftitems>
---->

do
	for _, VAL in pairs({"1", "5", "10", "25", "50", "100"}) do
		core.register_craftitem("engine:paper_with_value_of_"..VAL, {
			description = "$"..VAL,
			inventory_image = "minegeld_"..VAL..".png",
			stack_max = 65535,
			groups = {money=1, flammable = 1, paper = 1}
		})
	end
end

---->
--Helpers>
---->

local function ReturnReal(v)
	if not v then
		return 0
	end
	local a1 = tostring(v)
	if not a1:find("-") then
		return 0
	end
	local a2 = string.sub(a1, 2)
	local a3 = tonumber(a2)
	return a3
end

local C = core.colorize

local function SendAnnounce(pname, val, opt_text)
	if pname and val then
		if val > 0 then
			core.chat_send_player(pname, "[Bank] "..S("You received:").." "..C("green", "+$"..tostring(val))..": "..(opt_text or ""))
			return
		elseif val < 0 then
			core.chat_send_player(pname, "[Bank] "..S("You had a expense: ").." "..C("red", "-$"..tostring(ReturnReal(val)))..": "..(opt_text or ""))
			return
		elseif val == 0 then
			core.chat_send_player(pname, "[Bank] "..S("Bank balance not modified."))
		end
	end
end

local function ReturnColorizedBalance(balance)
	local b = tonumber(balance)
	local color = "white"
	if b then
		if b < 70 then
			color = "red"
		elseif b < 130 then
			color = "orange"
		elseif b < 250 then
			color = "yellow"
		elseif b < 360 then
			color = "#7CFF00"
		elseif b > 360 then
			color = "green"
		end
		return C(color, "$"..tostring(b))
	end
	return C(color, "$0")
end

---->
--Queues>
---->

function MineStars.Bank.SavePayingForLater(payer,to,amount,reason)
	--Check Storage
	local data = MineStars.Storage:get_string("payers"..Name(to))
	local dec = core.deserialize(data)
	if not dec then dec = {} end
	table.insert(dec, {Payer = Name(payer), Amount = amount, Reason = reason})
	local ced = core.serialize(dec)
	MineStars.Storage:set_string("payers"..Name(to), ced)
end

function MineStars.Bank.QueueRmvTo(to, amount)
	if Name(to) and IsOnline(Name(to)) then
		MineStars.Bank.RmvValue(to, amount)
	end
	local data = MineStars.Storage:get_string("rmvp"..Name(to))
	local dec = core.deserialize(data)
	if not dec then dec = {} end
	table.insert(dec, {Amount = amount, Reason = reason})
	local ced = core.serialize(dec)
	MineStars.Storage:set_string("payers"..Name(to), ced)
end

function MineStars.Bank.QueuePayTo(payer, to, amount, force, reason)
	MineStars.Bank.RmvValue(payer, amount)
	if force or IsOnline(to) then
		
		--MineStars.Bank.QueueRmvTo(payer, amount)
		MineStars.Bank.AddValue(to, amount)
		SendAnnounce(Name(payer), -amount, (reason and S(reason, Name(payer))) or (S("By giving to @1 money", Name(to))))
		SendAnnounce(Name(to), amount, (reason and S(reason, Name(payer))) or (S("By @1", Name(payer))))
		return core.colorize("lightgreen", "➤ ✔")
	end
	MineStars.Bank.SavePayingForLater(payer, to, amount, reason)
	return core.colorize("lightgreen", "➤ "..S("Queued!"))
end

core.register_on_joinplayer(function(player)
	local n = Name(player)
	local data = MineStars.Storage:get_string("payers"..n)
	local dec = core.deserialize(data) or {}
	if dec then
		for _, dt in pairs(dec) do
			MineStars.Bank.QueuePayTo(dt.Payer, n, dt.Amount, true, dt.Reason)
			dec[_] = nil
		end
	end
	MineStars.Storage:set_string("payers"..n, core.serialize(dec))
	MineStars.Bank.CachePos[n] = nil
	
	-- RMV [Exploits found!]
	--[[
	local data = MineStars.Storage:get_string("rmvp"..n)
	local dec = core.deserialize(data) or {}
	local amount = 0
	if dec then
		for _, dt in pairs(dec) do
			MineStars.Bank.RmvValue(player, dt.Amount)
			amount = amount + dt.Amount
		end
	end
	if amount > 0 then
		SendAnnounce(n, -amount, "Buying, Donating, etc")
	end
	MineStars.Storage:set_string("rmvp"..n, core.serialize(dec))--]]
end)

---->
--Commands>
---->

core.register_chatcommand("balance", {
	params = "",
	description = S("Get your actual money balance"),
	func = function(n)
		local p = Player(n)
		if p then
			local balance = MineStars.Bank.GetValue(n)
			return true, "➤ "..core.colorize(MineStars.Player.GetColor(p), n)..": "..ReturnColorizedBalance(balance)
		end
	end
})

core.register_chatcommand("add_balance", {
	params = "",
	description = S("Add balance"),
	func = function(n, prm)
		local params = tonumber(prm:split(" ")[1])
		local p = Player(n)
		if p and params and type(params) == "number" then
			MineStars.Bank.AddValue(p, params)
			SendAnnounce(n, params)
			--return true, "➤ "..core.colorize(MineStars.Player.GetColor(p), n)..": "..ReturnColorizedBalance(balance)
		end
	end
})

for _, idkhowicannamethisvariable_imfreaky in pairs({"pay", "pagar", "donate", "donar"}) do
	core.register_chatcommand(idkhowicannamethisvariable_imfreaky, {
		params = "<player> <$$$>",
		description = S("Pay or donate a player"),
		func = function(n, p)
			local params = p:split(" ")
			if params[1] and params[2] then
				--p1: name
				--p2: money-amount
				return true, MineStars.Bank.QueuePayTo(Player(name), params[1], tonumber(params[2]))
			else
				return true, core.colorize("#FF7C7C", X..MineStars.Translator"Invalid use of the command!")
			end
		end
	})
end

--Shop OBJ ["[combine:32x32:0,0=default_cobble.png:0,16=default_ice.png:16,0=default_dirt.png:16,16=default_grass.png"]
core.register_entity(":bank:shop_itm",{
	initial_properties = {
		hp_max = 1,
		visual = "wielditem",
		visual_size = {x = 0.20, y = 0.20},
		collisionbox = {0, 0, 0, 0, 0, 0},
		physical=false,
		textures = {""},
		static_save = true,
	},
	bankshop = true,
	item = "",
	on_activate = function(self, st)
		self.item = self.item ~= "" or st
		self.object:set_properties({textures = {[1] = st or self.item or "ignore"}})
	end,
	get_staticdata = function(self)
		if self.item then
			return self.item
		end
	end,
})

local DPOS = {
	{
		{x=0.2,y=0.2,z=0},
		{x=-0.2,y=0.2,z=0},
		{x=0.2,y=-0.2,z=0},
		{x=-0.2,y=-0.2,z=0}
	},
	{
		{x=0,y=0.2,z=0.2},
		{x=0,y=0.2,z=-0.2},
		{x=0,y=-0.2,z=0.2},
		{x=0,y=-0.2,z=-0.2}
	},
	{
		{x=-0.2,y=0.2,z=0},
		{x=0.2,y=0.2,z=0},
		{x=-0.2,y=-0.2,z=0},
		{x=0.2,y=-0.2,z=0}
	},
	{
		{x=0,y=0.2,z=-0.2},
		{x=0,y=0.2,z=0.2},
		{x=0,y=-0.2,z=-0.2},
		{x=0,y=-0.2,z=0.2}
	}
}

local PDIR = {
	{x=0,y=0,z=-1},
	{x=-1,y=0,z=0},
	{x=0,y=0,z=1},
	{x=1,y=0,z=0}
}

function MineStars.Bank.SetupObject(pos)
	local meta = core.get_meta(pos)
	if meta then
		local inv = meta:get_inventory()
		local list = inv:get_list("showarea")
		local node = core.get_node(pos)
		local param2 = node.param2
		local pos_ = table.copy(PDIR[param2+1])
		pos.x = pos.x + pos_.x*0.01
		pos.y = pos.y + pos_.y*6.5/16
		pos.z = pos.z + pos_.z*0.01
		--print(((function(list) local to_return = 0; for I, element in pairs(list) do if element:get_count() > 0 then to_return = I; end; end; return to_return or 1; end)(list) or 1))
		for i = 1, ((function(list) local to_return = 0; for I, element in pairs(list) do if element:get_count() > 0 then to_return = I; end; end; return to_return or 1; end)(list) or 1) do
			local ITM = list[i]
			local obj = core.add_entity(vector.add(pos, DPOS[param2+1][i]), "bank:shop_itm")
			obj:set_properties({textures = {[1] = ITM:get_name()}})
			obj:set_yaw(math.pi*2 - node.param2 * math.pi/2)
			if obj:get_luaentity() then
				obj:get_luaentity().item = ITM:get_name()
			end
		end
	end
end

function MineStars.Bank.UpdateObjects(pos)
	for _, obj in pairs(core.get_objects_inside_radius(pos, 1.2)) do
		local ent = obj:get_luaentity()
		if ent then
			if ent.bankshop then
				obj:remove()
			end
		end
	end
	MineStars.Bank.SetupObject(pos)
end

core.register_node("engine:shop", {
	description = "Shop",
	tiles = {"shop_table2.png", "shop_table2.png", "shop_table2.png", "shop_table2.png", "shop_table2.png", "shop_table1.png"},
	groups = {choppy = 2, oddly_breakable_by_hand = 1,tubedevice = 1, tubedevice_receiver = 1,mesecon=2},
	drawtype = "nodebox",
	node_box = {type = "fixed", fixed={-0.5,-0.5,0.0,0.5,0.5,0.5}},
	paramtype2 = "facedir",
	paramtype = "light",
	sunlight_propagates = true,
	light_source = 10,
	tube = {
		insert_object = function(pos, node, stack, direction)
			local meta = core.get_meta(pos)
			local inv = meta:get_inventory()
			local added = inv:add_item("main", stack)
			return added
		end,
		can_insert = function(pos, node, stack, direction)
			local meta = core.get_meta(pos)
			local inv = meta:get_inventory()
			return inv:room_for_item("main", stack)
		end,
		input_inventory = "main",
		connect_sides = {left = 1, right = 1, front = 1, back = 1, top = 1, bottom = 1}
	},
	after_place_node = function(pos, placer)
		local meta = core.get_meta(pos)
		meta:set_string("owner", Name(placer))
		--See if player has creative [Sonic CD menu start US]
		if core.check_player_privs(Name(placer), {creative=true}) then
			meta:set_int("creative_shop", 1)
		end
		meta:set_string("infotext", "Shop by: " .. Name(placer))
		if core.check_player_privs(Name(placer), {creative=true}) then
			meta:set_int("creative", 1)
		end
		meta:set_string("prices", "return {}")
	end,
	on_construct = function(pos)
		local meta=core.get_meta(pos)
		meta:get_inventory():set_size("main", 32)
		meta:get_inventory():set_size("showarea", 4)
	end,
	on_rightclick = function(pos, node, player, itemstack, pointed_thing)
		if core.get_meta(pos):get_string("owner") == player:get_player_name() then
			MineStars.Bank.ShowToPlayerConfigShop(player, pos)
		else
			if core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
				MineStars.Bank.ShowToPlayerConfigShop(player, pos)
			else
				MineStars.Bank.ShowToPlayerShop(player, pos)
			end
		end
	end,
	allow_metadata_inventory_put = function(pos, listname, index, stack, player)
		if stack:get_wear()==0 and (core.get_meta(pos):get_string("owner")==player:get_player_name() or core.check_player_privs(player:get_player_name(), {protection_bypass=true})) then
			if listname ~= "showarea" then
				return stack:get_count()
			else
				Inv(player):add_item("main", stack)
				return stack:get_count()
			end
		end
		return 0
	end,
	allow_metadata_inventory_take = function(pos, listname, index, stack, player)
		if core.get_meta(pos):get_string("owner") == player:get_player_name() or core.check_player_privs(player:get_player_name(), {protection_bypass=true}) then
			--if listname--2629
			if listname ~= "showarea" then
				return stack:get_count()
			else
				local meta = core.get_meta(pos)
				local inv = meta:get_inventory()
				if inv then
					inv:set_stack(listname, index, ItemStack(""))
				end
				return 0
			end
		end
		return 0
	end,
	allow_metadata_inventory_move = function(pos, from_list, from_index, to_list, to_index, count, player)
		if core.check_player_privs(player:get_player_name(), {protection_bypass=true}) or core.get_meta(pos):get_string("owner")==player:get_player_name() then
			if from_list == "showarea" then
				return -1
			elseif to_list == "showarea" then
				local inv = core.get_meta(pos):get_inventory()
				local stack = inv:get_stack(from_list, from_index)
				stack:set_count(count)
				inv:set_stack("showarea", to_index, stack)
				return 0
			end
			return count
		end
		return 0
	end,
	can_dig = function(pos, player)
		local meta=core.get_meta(pos)
		local inv=meta:get_inventory()
		if ((meta:get_string("owner") == player:get_player_name()) or core.check_player_privs(player:get_player_name(), {protection_bypass=true})) and inv:is_empty("main") and meta:get_string("owner")=="" then
			--smartshop.update(pos,"clear")
			return true
		end
	end,
})

core.register_on_player_receive_fields(function(player, frm, fie)
	if frm == "bank:shop" then
		for I=1,4 do
			if fie["itm"..I] then
				MineStars.Bank.AcquireItem(player, I)
			end
		end
	elseif frm == "bank:cfgshop" then
		--if fie.key_enter_field
		local pos = MineStars.Bank.CachePos[Name(player)]
		local meta = core.get_meta(pos)
		if meta then
			local prices = core.deserialize(meta:get_string("prices")) or {}
			if fie.itm1_price == nil then
				fie.itm1_price = prices[1] or 0
			end
			if fie.itm2_price == nil then
				fie.itm2_price = prices[2] or 0
			end
			if fie.itm3_price == nil then
				fie.itm3_price = prices[3] or 0
			end
			if fie.itm4_price == nil then
				fie.itm4_price = prices[4] or 0
			end
			do
				local f0 = tonumber(fie.itm1_price) or 65535
				local f1 = tonumber(fie.itm2_price) or 65535
				local f2 = tonumber(fie.itm3_price) or 65535
				local f3 = tonumber(fie.itm4_price) or 65535
				if f0 == 65535 or f1 == 65535 or f2 == 65535 or f3 == 65535 then
					core.chat_send_player(Name(player), core.colorize("#FF8E8E", "✗"..S("Check the prices, make sure the '$' is not in the fields!")))
					return
				end
				local t = {
					[1] = f0,
					[2] = f1,
					[3] = f2,
					[4] = f3,
				}
				meta:set_string("prices", core.serialize(t))
			end
		end
		if fie.key_enter_field or fie.quit then
			core.chat_send_player(Name(player), core.colorize("lightgreen", "✔"..S("Saved successfully!")))
			MineStars.Bank.UpdateObjects(pos)
		end
	end
end)

for _, str in pairs({"cobrar", "withdraw"}) do
	core.register_chatcommand("withdraw", {
		params = "<$$$>",
		description = MineStars.Translator("Withdraw your money"),
		func = BuildCmdFuncFor(function(name, moneyamount)
			if Player(name) then
				local amount = tonumber(moneyamount or 1)
				local money = MineStars.Bank.GetValue(name)
				if money >= amount then
					if amount >= 100 then
						for i = 0, amount, 100 do
							if i ~= 0 then
								core.chat_send_player(name, core.colorize("lightgreen", "✔"..S("-> $100")))
								MineStars.Bank.RmvValue(name, 100)
								Inv(name):add_item("main", ItemStack("engine:paper_with_value_of_"..100))
							end
						end
					elseif amount >= 50 and amount < 100 then
						for i = 0, amount, 50 do
							if i ~= 0 then
								core.chat_send_player(name, core.colorize("lightgreen", "✔"..S("-> $50")))
								MineStars.Bank.RmvValue(name, 50)
								Inv(name):add_item("main", ItemStack("engine:paper_with_value_of_"..50))
							end
						end
					elseif amount >= 25 and amount < 50 then
						for i = 0, amount, 25 do
							if i ~= 0 then
								core.chat_send_player(name, core.colorize("lightgreen", "✔"..S("-> $25")))
								MineStars.Bank.RmvValue(name, 25)
								Inv(name):add_item("main", ItemStack("engine:paper_with_value_of_"..25))
							end
						end
					elseif amount >= 10 and amount < 25 then
						for i = 0, amount, 10 do
							if i ~= 0 then
								core.chat_send_player(name, core.colorize("lightgreen", "✔"..S("-> $10")))
								MineStars.Bank.RmvValue(name, 10)
								Inv(name):add_item("main", ItemStack("engine:paper_with_value_of_"..10))
							end
						end
					elseif amount >= 5 and amount < 10 then
						for i = 0, amount, 5 do
							if i ~= 0 then
								core.chat_send_player(name, core.colorize("lightgreen", "✔"..S("-> $5")))
								MineStars.Bank.RmvValue(name, 5)
								Inv(name):add_item("main", ItemStack("engine:paper_with_value_of_"..5))
							end
						end
					elseif amount == 1 then
						for i = 0, amount, 1 do
							if i ~= 0 then
								core.chat_send_player(name, core.colorize("lightgreen", "✔"..S("-> $1")))
								MineStars.Bank.RmvValue(name, 1)
								Inv(name):add_item("main", ItemStack("engine:paper_with_value_of_"..1))
							end
						end
					else
						return true, core.colorize("#FF8E8E", "✗"..S("Invalid amount!"))
					end
				else
					return true, core.colorize("#FF8E8E", "✗"..S("No enough money in your account!"))
				end
			else
				return true, core.colorize("#FF8E8E", "✗"..S("You aren't connected!"))
			end
		end)
	})
end

--Random drops
local max = 300

local timer = 0



--Withdraw/Saves node

--Bank Function "Main Formspec"
local function return_formspec(playername, cc)
	return "size[4,2.2]" ..
	"box[-0.3,-0.3;4.37,0.3;#55AA11]" ..
	"label[-0.2,-.39;"..playername.." with $"..cc.."]" ..
	"style[withdraw;bgcolor=lightgreen;textcolor=white]" ..
	"style[deposit;bgcolor=lightblue;textcolor=white]" ..
	"style[exit;bgcolor=red;textcolor=white]" ..
	"image_button[-0.2,0.1;4.45,1;blank.png;withdraw;Withdraw]" ..
	"image_button[-0.2,1;4.45,1;blank.png;deposit;Deposit]" ..
	"image_button_exit[-0.2,1.9;4.45,0.8;blank.png;exit;Exit]"
end

core.register_node("engine:atm", {
	description = "ATM",
	tiles = {
		"atm_top.png", "atm_top.png",
		"atm_side.png", "atm_side.png",
		"atm_side.png", "atm_front.png"
	},
	paramtype2 = "facedir",
	groups = {cracky=2, bank_equipment = 3},
	legacy_facedir_simple = true,
	is_ground_content = false,
	sounds = default.node_sound_stone_defaults(),
	on_rightclick = function(_, _, player)
		core.show_formspec(Name(player), "bank:_menu", return_formspec(Name(player), MineStars.Bank.GetValue(player)))
	end,
})

--Other Formspec & Functions

--Deposit
local return_deposit_formspec = function(balance)
	return "size[4.01,3.5]" ..
	"box[1.94,0.2;2.24,3.81;#00FF81]" ..
	"box[-0.3,0.2;2.16,3.81;#0000FF]" ..
	"box[-0.3,-0.32;4.45,0.52;#008DFF]" ..
	"label[-0.22,-0.32;"..MineStars.Translator("Deposit").."\\, Balance: "..balance.."]" ..
	"image_button[-0.22,0.28;1.17,1.09;blank.png;w_one;$1]" ..
	"image_button[-0.22,1.32;1.17,1.09;blank.png;w_five;$5]" ..
	"image_button[-0.22,2.36;1.17,1.09;blank.png;w_eleven;$10]" ..
	"image_button[0.82,2.36;1.17,1.09;blank.png;w_veinticinco;$25]" ..
	"image_button[0.82,0.28;1.17,1.09;blank.png;w_cincuenta;$50]" ..
	"image_button[0.82,1.32;1.17,1.09;blank.png;w_cien;$100]" ..
	"image_button[2.02,0.28;1.17,1.09;blank.png;w_one_max;$1 All]" ..
	"image_button[2.02,1.32;1.17,1.09;blank.png;w_five_max;$5 All]" ..
	"image_button[2.02,2.36;1.17,1.09;blank.png;w_eleven_max;$10 All]" ..
	"image_button[3.06,2.36;1.17,1.09;blank.png;w_veinticinco_max;$25 All]" ..
	"image_button[3.06,1.32;1.17,1.09;blank.png;w_cien_max;$100\nAll]" ..
	"image_button[3.06,0.28;1.17,1.09;blank.png;w_cincuenta_max;$50\nAll]" ..
	"image_button_exit[-0.22,3.4;4.45,0.57;blank.png;exit;Cancel]"
end

--Withdraw
local return_withdraw_formspec = function(balance)
	return "size[4.01,3.5]" ..
	"box[1.94,0.2;2.24,3.81;#00FF81]" ..
	"box[-0.3,0.2;2.16,3.81;#0000FF]" ..
	"box[-0.3,-0.32;4.45,0.52;#008DFF]" ..
	"label[-0.22,-0.32;"..MineStars.Translator("Withdraw").."\\, Balance: "..balance.."]" ..
	"image_button[-0.22,0.28;1.17,1.09;blank.png;w_one;$1]" ..
	"image_button[-0.22,1.32;1.17,1.09;blank.png;w_five;$5]" ..
	"image_button[-0.22,2.36;1.17,1.09;blank.png;w_eleven;$10]" ..
	"image_button[0.82,2.36;1.17,1.09;blank.png;w_veinticinco;$25]" ..
	"image_button[0.82,0.28;1.17,1.09;blank.png;w_cincuenta;$50]" ..
	"image_button[0.82,1.32;1.17,1.09;blank.png;w_cien;$100]" ..
	"image_button[2.02,0.28;1.17,1.09;blank.png;w_one_max;$1 All]" ..
	"image_button[2.02,1.32;1.17,1.09;blank.png;w_five_max;$5 All]" ..
	"image_button[2.02,2.36;1.17,1.09;blank.png;w_eleven_max;$10 All]" ..
	"image_button[3.06,2.36;1.17,1.09;blank.png;w_veinticinco_max;$25 All]" ..
	"image_button[3.06,1.32;1.17,1.09;blank.png;w_cien_max;$100\nAll]" ..
	"image_button[3.06,0.28;1.17,1.09;blank.png;w_cincuenta_max;$50\nAll]" ..
	"image_button_exit[-0.22,3.4;4.45,0.57;blank.png;exit;Cancel]"
end

--Helper
local function HasEnough(player, amount)
	local player_money = MineStars.Bank.GetValue(player)
	return player_money >= amount
end

--Functions
core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "bank:withdraw" then
		local inv = Inv(player)
		if fields.w_one then
			if HasEnough(player, 1) then
				if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_1")) then
					inv:add_item("main", ItemStack("engine:paper_with_value_of_1"))
					MineStars.Bank.RmvValue(player, 1)
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
					return
				end
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
			end
		elseif fields.w_one_max then
			local amount = MineStars.Bank.GetValue(player)
			for i = 0, amount, 1 do
				if HasEnough(player, 1) then
					if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_1")) then
						inv:add_item("main", ItemStack("engine:paper_with_value_of_1"))
						MineStars.Bank.RmvValue(player, 1)
					else
						core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
						return
					end
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
					return
				end
			end
		--FIVE SECTOR
		elseif fields.w_five then
			if HasEnough(player, 5) then
				if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_5")) then
					inv:add_item("main", ItemStack("engine:paper_with_value_of_5"))
					MineStars.Bank.RmvValue(player, 5)
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
					return
				end
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
			end
		elseif fields.w_five_max then
			local amount = MineStars.Bank.GetValue(player)
			for i = 0, amount, 5 do
				if HasEnough(player, 5) then
					if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_5")) then
						inv:add_item("main", ItemStack("engine:paper_with_value_of_5"))
						MineStars.Bank.RmvValue(player, 5)
					else
						core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
						return
					end
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
					return
				end
			end
		--ELEVEN SECTOR
		elseif fields.w_eleven then
			if HasEnough(player, 10) then
				if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_10")) then
					inv:add_item("main", ItemStack("engine:paper_with_value_of_10"))
					MineStars.Bank.RmvValue(player, 10)
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
					return
				end
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
			end
		elseif fields.w_eleven_max then
			local amount = MineStars.Bank.GetValue(player)
			for i = 0, amount, 10 do
				if HasEnough(player, 10) then
					if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_10")) then
						inv:add_item("main", ItemStack("engine:paper_with_value_of_10"))
						MineStars.Bank.RmvValue(player, 10)
					else
						core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
						return
					end
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
					return
				end
			end
		--TWENY FIVE SECTOR
		elseif fields.w_veinticinco then
			if HasEnough(player, 25) then
				if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_25")) then
					inv:add_item("main", ItemStack("engine:paper_with_value_of_25"))
					MineStars.Bank.RmvValue(player, 25)
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
					return
				end
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
			end
		elseif fields.w_veinticinco_max then
			local amount = MineStars.Bank.GetValue(player)
			for i = 0, amount, 25 do
				if HasEnough(player, 25) then
					if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_25")) then
						inv:add_item("main", ItemStack("engine:paper_with_value_of_25"))
						MineStars.Bank.RmvValue(player, 25)
					else
						core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
						return
					end
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
					return
				end
			end
		--50 SECTOR
		elseif fields.w_cincuenta then
			if HasEnough(player, 50) then
				if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_50")) then
					inv:add_item("main", ItemStack("engine:paper_with_value_of_50"))
					MineStars.Bank.RmvValue(player, 50)
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
					return
				end
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
			end
		elseif fields.w_cincuenta_max then
			local amount = MineStars.Bank.GetValue(player)
			for i = 0, amount, 50 do
				if HasEnough(player, 50) then
					if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_50")) then
						inv:add_item("main", ItemStack("engine:paper_with_value_of_50"))
						MineStars.Bank.RmvValue(player, 50)
					else
						core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
						return
					end
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
					return
				end
			end
		--100 SECTOR
		elseif fields.w_cien then
			if HasEnough(player, 100) then
				if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_100")) then
					inv:add_item("main", ItemStack("engine:paper_with_value_of_100"))
					MineStars.Bank.RmvValue(player, 100)
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
					return
				end
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
			end
		elseif fields.w_cien_max then
			local amount = MineStars.Bank.GetValue(player)
			for i = 0, amount, 100 do
				if HasEnough(player, 100) then
					if inv:room_for_item("main", ItemStack("engine:paper_with_value_of_100")) then
						inv:add_item("main", ItemStack("engine:paper_with_value_of_100"))
						MineStars.Bank.RmvValue(player, 100)
					else
						core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough space in your inventory!")))
						return
					end
				else
					core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left in bank!")))
					return
				end
			end
		end
		if not fields.quit then
			core.show_formspec(Name(player), "bank:withdraw", return_withdraw_formspec(MineStars.Bank.GetValue(player)))
		end
	elseif formname == "bank:deposit" then
		local inv = Inv(player)
		--One Loop
		if fields.w_one then
			if inv:contains_item("main", ItemStack("engine:paper_with_value_of_1")) then	
				inv:remove_item("main", ItemStack("engine:paper_with_value_of_1"))
				MineStars.Bank.AddValue(player, 1)
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left on the inventory")))
				return
			end
		elseif fields.w_five then
			if inv:contains_item("main", ItemStack("engine:paper_with_value_of_5")) then	
				inv:remove_item("main", ItemStack("engine:paper_with_value_of_5"))
				MineStars.Bank.AddValue(player, 5)
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left on the inventory")))
				return
			end
		elseif fields.w_eleven then
			if inv:contains_item("main", ItemStack("engine:paper_with_value_of_10")) then	
				inv:remove_item("main", ItemStack("engine:paper_with_value_of_10"))
				MineStars.Bank.AddValue(player, 10)
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left on the inventory")))
				return
			end
		elseif fields.w_veinticinco then
			if inv:contains_item("main", ItemStack("engine:paper_with_value_of_25")) then	
				inv:remove_item("main", ItemStack("engine:paper_with_value_of_25"))
				MineStars.Bank.AddValue(player, 25)
			else
				core.chat_send_player(Name(player), Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left on the inventory")))
				return
			end
		elseif fields.w_cincuenta then
			if inv:contains_item("main", ItemStack("engine:paper_with_value_of_50")) then	
				inv:remove_item("main", ItemStack("engine:paper_with_value_of_50"))
				MineStars.Bank.AddValue(player, 50)
			else
				core.chat_send_player(Name(player), Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left on the inventory")))
				return
			end
		elseif fields.w_cien then
			if inv:contains_item("main", ItemStack("engine:paper_with_value_of_100")) then	
				inv:remove_item("main", ItemStack("engine:paper_with_value_of_100"))
				MineStars.Bank.AddValue(player, 100)
			else
				core.chat_send_player(Name(player), core.colorize("#FF7C7C", X..MineStars.Translator("No enough money left on the inventory")))
				return
			end
		--Multiple loops
		elseif fields.w_one_max then
			local item_count = 0
			for I, ITM in pairs(inv:get_list("main")) do
				if ITM:get_name() == "engine:paper_with_value_of_1" then
					item_count = ITM:get_count() + item_count
				end
			end
			inv:remove_item("main", ItemStack("engine:paper_with_value_of_1 "..item_count))
			MineStars.Bank.AddValue(player, 1 * item_count)
		elseif fields.w_five_max then
			local item_count = 0
			for I, ITM in pairs(inv:get_list("main")) do
				if ITM:get_name() == "engine:paper_with_value_of_5" then
					item_count = ITM:get_count() + item_count
				end
			end
			inv:remove_item("main", ItemStack("engine:paper_with_value_of_5 "..item_count))
			MineStars.Bank.AddValue(player, 5 * item_count)
		elseif fields.w_eleven_max then
			local item_count = 0
			for I, ITM in pairs(inv:get_list("main")) do
				if ITM:get_name() == "engine:paper_with_value_of_10" then
					item_count = ITM:get_count() + item_count
				end
			end
			inv:remove_item("main", ItemStack("engine:paper_with_value_of_10 "..item_count))
			MineStars.Bank.AddValue(player, 10 * item_count)
		elseif fields.w_veinticinco_max then
			local item_count = 0
			for I, ITM in pairs(inv:get_list("main")) do
				if ITM:get_name() == "engine:paper_with_value_of_25" then
					item_count = ITM:get_count() + item_count
				end
			end
			inv:remove_item("main", ItemStack("engine:paper_with_value_of_25 "..item_count))
			MineStars.Bank.AddValue(player, 25 * item_count)
		elseif fields.w_cincuenta_max then
			local item_count = 0
			for I, ITM in pairs(inv:get_list("main")) do
				if ITM:get_name() == "engine:paper_with_value_of_50" then
					item_count = ITM:get_count() + item_count
				end
			end
			inv:remove_item("main", ItemStack("engine:paper_with_value_of_50 "..item_count))
			MineStars.Bank.AddValue(player, 50 * item_count)
		elseif fields.w_cien_max then
			local item_count = 0
			for I, ITM in pairs(inv:get_list("main")) do
				if ITM:get_name() == "engine:paper_with_value_of_100" then
					item_count = ITM:get_count() + item_count
				end
			end
			inv:remove_item("main", ItemStack("engine:paper_with_value_of_100 "..item_count))
			MineStars.Bank.AddValue(player, 100 * item_count)
		end
		if not fields.quit then
			core.show_formspec(Name(player), "bank:withdraw", return_deposit_formspec(MineStars.Bank.GetValue(player)))
		end
	elseif formname == "bank:_menu" then
		if fields.deposit then
			core.show_formspec(Name(player), "bank:deposit", return_deposit_formspec(MineStars.Bank.GetValue(player)))
			return
		end
		if fields.withdraw then
			core.show_formspec(Name(player), "bank:withdraw", return_withdraw_formspec(MineStars.Bank.GetValue(player)))
			return
		end
	end
end)
















































