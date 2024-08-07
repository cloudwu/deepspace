local util = require "util"
local allocid = require "gameplay.allocid"

return function (inst)
	local actor = {}
	inst.actor = actor

	local actor_init = util.map_from_list({
		"building",
		"worker",
	}, function(name)
		local meta = require ("gameplay.actor_" .. name) (inst)
		meta.__index = meta
		return meta
	end)

	local alloc_id = allocid()
	local actors = {}

	local new_set = {}
	local delete_set = {}

	function actor.new(init)
		local id = alloc_id(init.id)
		init.id = id
		local meta = assert(actor_init[init.name])
		local obj = setmetatable(init, meta)
		obj:init()
		new_set[id] = obj
		return id
	end

	function actor.update()
		-- move new_set to actors
		for id, obj in pairs(new_set) do
			actors[id] = obj
			new_set[id] = nil
		end
		-- update all
		local n = 0
		for id , obj in pairs(actors) do
			if obj:update() then
				n = n + 1
				delete_set[n] = id
			end
		end
		-- remove deleted actors
		for i = 1, n do
			actors[delete_set[i]] = nil
			delete_set[i] = nil
		end
	end

	local function send_message(obj, msg, ...)
		local f = obj[msg] or ("Invalid message" .. msg)
		f(obj, ...)
	end

	function actor.publish(msg, ...)
		for _, obj in pairs(actors) do
			local f = obj[msg]
			if f then
				f(obj, ...)
			end
		end
		for _, obj in pairs(new_set) do
			local f = obj[msg]
			if f then
				f(obj, ...)
			end
		end
	end

	function actor.message(id, ...)
		local obj = actors[id] or new_set[id] or ("No actor " .. id)
		send_message(obj, ...)
	end
	
	function actor.export(file)
		local list = {}
		actor.publish("export", list)
		if next(list) then
			file:write_list("actor", list)
		end
	end
	
	function actor.import(savedata)
		alloc_id = allocid()
		actors = {}
		if savedata.actor then
			for _, init in ipairs(savedata.actor) do
				actor.new(init)
			end
			new_set, actors = actors, new_set
		end
	end

	return actor
end
