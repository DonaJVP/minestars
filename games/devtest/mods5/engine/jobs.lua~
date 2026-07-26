MineStars.Jobs = {
	RegisteredJobs = {
		--[[Woodcutter = {
			Title = MineStars.Translator("Woodcutter"),
			OnApply = function(player)
				--On apply to meta, like he is registered to the job
				local meta = player:get_meta()
				if meta then
					local data = core.deserialize(meta:get_string("jobs_apply")) or {}
					if not data.Woodcutter then
						data.Woodcutter = true
					end
				end
			end,
			OnLeave = function(player)
				
			end,
			OnApplyString = "You are a Woodcutter now!",
			Type = {
				Type = "OnDigNode",
				Node = "group:wood",
				Earning = {Every = 3, Amount = 5},
			},
		}--]]
	},
	OnDigNodeJobs = {},
	OnPlaceNodeJobs = {},
	OnCraftJobs = {},
	Cache = {},
}

local function get_count(t)
	local c = 1
	for _ in pairs(t) do
		c = c + 1
	end
	return c
end

function MineStars.Jobs.Apply(player, job)
	player = Player(player)
	if player then
		local meta = player:get_meta()
		local data = core.deserialize((meta:get_string("jobs") ~= "" and meta:get_string("jobs")) or "return {}")
		print(get_count(data))
		if get_count(data) <= 4 then
			if not data[job] then
				data[job] = true
				MineStars.Jobs.Cache[Name(player)] = table.copy(data)
				if MineStars.Jobs.RegisteredJobs[job].OnApply then
					MineStars.Jobs.RegisteredJobs[job].OnApply(player)
				end
				meta:set_string("jobs", core.serialize(data))
				return true, check..MineStars.Translator(MineStars.Jobs.RegisteredJobs[job].OnApplyString or "You are a").." "..MineStars.Jobs.RegisteredJobs[job].Title
			else
				return false, "✗"..MineStars.Translator("You already are a @1", MineStars.Jobs.RegisteredJobs[job].Title)
			end
		else
			return false, "✗"..MineStars.Translator("You can't apply more jobs, enough jobs (Limit is 4 jobs)")
		end
	else
		return false, "✗"..MineStars.Translator("Must be connected!")
	end
end

function MineStars.Jobs.Remove(player, job)
	player = Player(player)
	if player then
		local meta = player:get_meta()
		if meta then
			local data = core.deserialize((meta:get_string("jobs") ~= "" and meta:get_string("jobs")) or "return {}")
			if data[job] then
				data[job] = nil
				if MineStars.Jobs.RegisteredJobs[job].OnLeave then
					MineStars.Jobs.RegisteredJobs[job].OnLeave(player)
				end
				MineStars.Jobs.Cache[Name(player)] = table.copy(data)
				meta:set_string("jobs", core.serialize(data))
				return true, check..MineStars.Translator("Done!")
			else
				return true, X..MineStars.Translator("You aren't a @1", job)
			end
		end
	end
end

function MineStars.Jobs.Get(player)
	player = Player(player)
	if player then
		local meta = player:get_meta()
		if meta then
			--print(dump(MineStars.Jobs.Cache[Name(player)] or (function(player) local meta = player:get_meta(); MineStars.Jobs.Cache[Name(player)] = core.deserialize((meta:get_string("jobs") ~= "" and meta:get_string("jobs")) or "return {}"); return core.deserialize((meta:get_string("jobs") ~= "" and meta:get_string("jobs")) or "return {}"); end)(player)))
			return MineStars.Jobs.Cache[Name(player)] or (function(player) local meta = player:get_meta(); MineStars.Jobs.Cache[Name(player)] = core.deserialize((meta:get_string("jobs") ~= "" and meta:get_string("jobs")) or "return {}"); return core.deserialize((meta:get_string("jobs") ~= "" and meta:get_string("jobs")) or "return {}"); end)(player)
		end
	end
end

function MineStars.Jobs.RegisterJob(name, def)
	MineStars.Jobs.RegisteredJobs[name] = def
	if def.Type and def.Type.Type == "OnDigNode" then
		MineStars.Jobs.OnDigNodeJobs[name] = true
	elseif def.Type and def.Type.Type == "OnPlaceNode" then
		MineStars.Jobs.OnPlaceNodeJobs[name] = true
	elseif def.Type and def.Type.Type == "OnCraft" then
		MineStars.Jobs.OnCraftJobs[name] = true
	end
end

--Helper
GroupToRawString = function(_stgroup)
	return _stgroup:split(":")[2] or ""
end
IsInGroupOrEquals = function(nodename_or_group, nodetable)
	if nodename_or_group == nodetable.name then return true end
	local def = core.registered_items[nodetable.name]
	--print(nodename_or_group, GroupToRawString(nodename_or_group))
	return (def and def.groups and def.groups[GroupToRawString(nodename_or_group)] and def.groups[GroupToRawString(nodename_or_group)] ~= nil) or false
end

--PlayerSide
function MineStars.Jobs.CanSendNotification(player)
	return (player:get_meta():get_string("jobs_can_send_notification_to_chat") == "yes" or player:get_meta():get_string("jobs_can_send_notification_to_chat") == "no" and false) or (function(player) if player:get_meta():get_string("jobs_can_send_notification_to_chat") == "no" then return false; end; player:get_meta():set_string("jobs_can_send_notification_to_chat", "yes"); core.chat_send_player(Name(player), core.colorize("lightgreen", arrow..MineStars.Translator("Jobs might send you notifications via chat, you can disable it with the command /disable_chat_notif"))); return true; end)(player)
end
function MineStars.Jobs.NotifyEarnedMoney(player, amount, _)
	if MineStars.Jobs.CanSendNotification(player) then
		core.chat_send_player(Name(player), core.colorize("lightgreen", MineStars.Translator("You earned $@1 from @2", tostring(amount), _)))
	end
end

--API

--OnDigNode
core.register_on_dignode(function(pos, oldnode, digger)
	if digger then
		local meta = digger:get_meta()
		if meta then
			local jobs = MineStars.Jobs.Get(digger)
			--print(dump(jobs))
			for jobname in pairs(jobs) do
				--print("__")
				if MineStars.Jobs.OnDigNodeJobs[jobname] then
					--print("loaded")
					if MineStars.Jobs.RegisteredJobs[jobname].Type.Node and IsInGroupOrEquals(MineStars.Jobs.RegisteredJobs[jobname].Type.Node, oldnode) then
						if MineStars.Jobs.RegisteredJobs[jobname].Type.Earning then
							local Every = (MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Every > 0 and MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Every) or 1
							local Amount = MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Amount or 5
							local CountedOnMeta = (meta:get_int("jobcount:"..jobname) > 0 and meta:get_int("jobcount:"..jobname)) or 0
							CountedOnMeta = CountedOnMeta + 1
							if CountedOnMeta >= Every then
								MineStars.Bank.AddValue(digger, Amount)
								MineStars.Jobs.NotifyEarnedMoney(digger, Amount, MineStars.Jobs.RegisteredJobs[jobname].Title)
								meta:set_int("jobcount:"..jobname, 0)
							else
								meta:set_int("jobcount:"..jobname, CountedOnMeta)
							end
						end
					end
				end
			end
		end
	end
end)

--OnPlaceNode
core.register_on_placenode(function(pos, newnode, placer, oldnode, itemstack, pointed_thing)
	if placer then
		local meta = placer:get_meta()
		if meta then
			local jobs = MineStars.Jobs.Get(placer)
			for jobname in pairs(jobs) do
				if MineStars.Jobs.OnPlaceNodeJobs[jobname] then
					if MineStars.Jobs.RegisteredJobs[jobname].Type.Node and IsInGroupOrEquals(MineStars.Jobs.RegisteredJobs[jobname].Type.Node, oldnode) then
						if MineStars.Jobs.RegisteredJobs[jobname].Type.Earning then
							local Every = (MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Every > 0 and MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Every) or 1
							local Amount = MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Amount or 5
							local CountedOnMeta = (meta:get_int("jobcount:"..jobname) > 0 and meta:get_int("jobcount:"..jobname)) or 0
							CountedOnMeta = CountedOnMeta + 1
							if CountedOnMeta >= Every then
								MineStars.Bank.AddValue(placer, Amount)
								MineStars.Jobs.NotifyEarnedMoney(digger, Amount, MineStars.Jobs.RegisteredJobs[jobname].Title)
								meta:set_int("jobcount:"..jobname, 0)
							else
								meta:set_int("jobcount:"..jobname, CountedOnMeta)
							end
						end
					end
				end
			end
		end
	end
end)

--9536

--OnCraft
core.register_on_craft(function(itemstack, player, old_craft_grid, craft_inv)
	if placer then
		local meta = player:get_meta()
		if meta then
			local jobs = MineStars.Jobs.Get(player)
			for jobname in pairs(jobs) do
				if MineStars.Jobs.OnCraftJobs[jobname] then
					--if
						if MineStars.Jobs.RegisteredJobs[jobname].Type.Earning then
							local Every = (MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Every > 0 and MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Every) or 1
							local Amount = MineStars.Jobs.RegisteredJobs[jobname].Type.Earning.Amount or 5
							local CountedOnMeta = (meta:get_int("jobcount:"..jobname) > 0 and meta:get_int("jobcount:"..jobname)) or 0
							CountedOnMeta = CountedOnMeta + 1
							if CountedOnMeta >= Every then
								MineStars.Bank.AddValue(player, Amount)
								MineStars.Jobs.NotifyEarnedMoney(digger, Amount, MineStars.Jobs.RegisteredJobs[jobname].Title)
								meta:set_int("jobcount:"..jobname, 0)
							else
								meta:set_int("jobcount:"..jobname, CountedOnMeta)
							end
						end
				end
			end
		end
	end
end)

--
-- Commands
--

for _, T in pairs({"jobs", "trabajo"}) do
	core.register_chatcommand(T, {
		params = (T == "jobs" and "<cmd> <job>") or "<comando> <trabajo>",
		description = MineStars.Translator("Manage your jobs"),
		func = BuildCmdFuncFor(function(name, cmd1, arg1)
			if Player(name) then
				--Apply | Aplucar
				if cmd1 == "apply" then
					if arg1 then
						if MineStars.Jobs.RegisteredJobs[arg1] then
							local bool, inf = MineStars.Jobs.Apply(Player(name), arg1)
							return true, (bool == false and core.colorize("#FF7C7C", inf)) or (bool and core.colorize("lightgreen", inf))
						else
							return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid job @1!", arg1))
						end
					else
						return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid use of the command!").."\n/jobs apply <job>")
					end
				elseif cmd1 == "aplicar" then
					if arg1 then
						if MineStars.Jobs.RegisteredJobs[arg1] then
							local bool, inf = MineStars.Jobs.Apply(Player(name), arg1)
							return true, (bool == false and core.colorize("#FF7C7C", inf)) or (bool and core.colorize("lightgreen", inf))
						else
							return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid job @1!", arg1))
						end
					else
						return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid use of the command!").."\n/jobs aplicar <trabajo>")
					end
				elseif cmd1 == "leave" then
					if arg1 then
						if MineStars.Jobs.RegisteredJobs[arg1] then
							local bool, inf = MineStars.Jobs.Remove(Player(name), arg1)
							return true, (bool == false and core.colorize("#FF7C7C", inf)) or (bool and core.colorize("lightgreen", inf))
						else
							return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid job @1!", arg1))
						end
					else
						return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid use of the command!").."\n/jobs leave <job>")
					end
				elseif cmd1 == "renunciar" then
					if arg1 then
						if MineStars.Jobs.RegisteredJobs[arg1] then
							local bool, inf = MineStars.Jobs.Remove(Player(name), arg1)
							return true, (bool == false and core.colorize("#FF7C7C", inf)) or (bool and core.colorize("lightgreen", inf))
						else
							return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid job @1!", arg1))
						end
					else
						return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid use of the command!").."\n/jobs renunciar <trabajo>")
					end
				elseif cmd1 == "ayuda" then
					return true, core.colorize("lightgreen", "➤ Menu de ayuda\n"..
					"✔ aplicar <trabajo>: Aplicar un trabajo\n"..
					"✔ renunciar [trabajo]: Renunciar a un trabajo (Es opcional especificar si solo tiene un trabajo activo)\n"..
					"✔ listar: Mostrar lista de trabajos disponibles\n"..
					"✔ listar <mios>: Mostrar lista de trabajos que estan activos [Actualmente trabajando]")
				elseif cmd1 == "help" then
					return true, core.colorize("lightgreen", "➤ Help menu\n"..
					"✔ apply <job>: Apply a job\n"..
					"✔ leave [job]: Leave a job [If you had only one job then don't specify the name of the job]\n"..
					"✔ list: Show a list of available jobs that you can work\n"..
					"✔ list <mine>: Show a list of jobs that you are working")
				elseif cmd1 == "list" then
					if arg1 == "mine" then
						local jobs = MineStars.Jobs.Get(Player(name))
						if jobs == {} then
							return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("You don't have any jobs"))
						else
							--local N = {}
							local str = MineStars.Translator("Jobs:").." "
							for job in pairs(jobs) do
								str = str .. MineStars.Jobs.RegisteredJobs[job].Title .. ", "
							end
							str = str..MineStars.Translator("end.")
							return true, core.colorize("lightgreen", "➤"..str)
						end
					else
						local jobs_ = MineStars.Jobs.Get(Player(name))
						local str = core.colorize("green", "➤"..MineStars.Translator("Jobs:")).." "
						for job in pairs(MineStars.Jobs.RegisteredJobs) do
							str = str .. ((jobs_[job] == nil and core.colorize("lightgreen", MineStars.Jobs.RegisteredJobs[job].Title)) or core.colorize("#FF7C7C", MineStars.Jobs.RegisteredJobs[job].Title)) .. ", "
						end
						str = str..core.colorize("green", MineStars.Translator("end."))
						return true, str
					end
				else
					return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Invalid use of the command!"))
				end
			else
				return true, core.colorize("#FF7C7C", "✗"..MineStars.Translator("Must be connected!"))
			end
		end),
	})
end


--
-- JOBS
--

--> OnDigNode

--Woodcutter [OnDigNode]
MineStars.Jobs.RegisterJob("woodcutter", {
	Title = MineStars.Translator("Woodcutter"),
	Type = {
		Type = "OnDigNode",
		Node = "group:tree",
		Earning = {
			Every = 3,
			Amount = 5,
		},
	}
})

--Digger [OnDigNode]
MineStars.Jobs.RegisterJob("digger", {
	Title = MineStars.Translator("Digger"),
	Type = {
		Type = "OnDigNode",
		Node = "default:dirt",
		Earning = {
			Every = 5,
			Amount = 5,
		},
	}
})

--Miner [OnDigNode]
MineStars.Jobs.RegisterJob("miner", {
	Title = MineStars.Translator("Miner"),
	Type = {
		Type = "OnDigNode",
		Node = "group:ore",
		Earning = {
			Every = 2,
			Amount = 4,
		},
	}
})


--> OnPlaceNode

MineStars.Jobs.RegisterJob("builder", {
	Title = MineStars.Translator("Builder"),
	Type = {
		Type = "OnPlaceNode",
		Node = "group:stone",
		Earning = {
			Every = 4,
			Amount = 8,
		},
	}
})

MineStars.Jobs.RegisterJob("torchplacer", {
	Title = MineStars.Translator("TorchPlacer"),
	Type = {
		Type = "OnPlaceNode",
		Node = "default:torch",
		Earning = {
			Every = 5,
			Amount = 3,
		},
	}
})




























