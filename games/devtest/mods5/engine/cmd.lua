cmd = {
}
function cmd.form(code, name)
	local formspec = "formspec_version[6]" ..
	"size[10.5,6]" ..
	"box[0,0;10.9,6.2;#FF6100]" ..
	"field[0.3,1;9.9,1.1;cmd;;core.chat_send_all(\"Hi!\")]" ..
	"label[3.4,0.5;Functions Caller]" ..
	"button_exit[0.2,4.8;5,1;exit;Quit]" ..
	"button[5.3,4.8;5,1;exec;Run]" ..
	"label[0.4,2.9;Function return:]"
	if type(code) == "table" or type(code) == "userdata" then
		formspec = formspec .. "label[0.3,3.8;Could not load a table/usrdata here! In chat is the table]"
		core.chat_send_player(name, dump(code))
	else
		formspec = formspec .. "label[0.3,3.8;("..tostring(code)..")]"
	end
	
	return formspec
end

function cmd.vform()
	local form = "formspec_version[6]" ..
	"size[10.5,4]" ..
	"box[0,0;10.5,4.1;#0075FF]" ..
	"label[3.2,0.3;Global variables modifier]" ..
	"field[0.1,0.8;10.3,0.8;varname;Variable name;default]" ..
	"field[0.1,2;10.3,0.8;var_content;New contents for variable;default]" ..
	"button[0.1,3;5.1,0.8;set;Set]" ..
	"button_exit[5.3,3;5.1,0.8;exit;Quit]"
	return form
end

function cmd.call(code)
	local func, err = loadstring(code)
	if not func then  -- Syntax error
		return err, true
	end
	local good, res = pcall(func)
	if not good then  -- Runtime error
		return res, false
	end
	return res, false
end

function cmd.show_gui(name, code)
	core.show_formspec(name, "cmd:main", cmd.form(code, name))
end

function cmd.show_vgui(name, code)
	core.show_formspec(name, "cmd:vars", cmd.vform())
end

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname == "cmd:main" then
		if fields.exec and fields.cmd then
			local result = cmd.call(fields.cmd)
			cmd.show_gui(player:get_player_name(), result)
		end
	elseif formname == "cmd:vars" then
		if fields.set and fields.var_content and fields.varname then
			_G[fields.varname] = fields.var_content
		end
	end
end)


core.register_chatcommand("exec", {
	description = "Run any function here",
	params = "<function>",
	privs = {server=true},
	func = function(name, params)
		if params == "" or params == " " or params == nil then
			cmd.show_gui(name)
		else
			local result = cmd.call(params)
			core.chat_send_player(name, dump(result))
		end
	end,
})

core.register_chatcommand("modvar", {
	description = "Modify global variables",
	params = "<var name> <new contents>",
	privs = {server=true},
	func = function(name, params)
		if params == "" or params == " " or params == nil then
			cmd.show_vgui(name)
		else
			local things = {}
			for thing in str:gmatch("%w+") do
				table.insert(things, thing)
			end
			if thing[1] and thing[2] then
				_G[thing[1]] = thing[2]
				core.chat_send_player(name, tostring(thing[1]) .. " = " .. tostring(thing[2]))
			end
		end
	end,
})



