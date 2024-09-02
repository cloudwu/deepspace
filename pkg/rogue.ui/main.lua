local ltask = require "ltask"
local M = {}

local ui_callback = {}
local ui_meta = {}

local init_callback
local function set_callback(window, what)
	window.onMessage(what, function(id, ...)
		ui_callback[id][what](...)	
	end)
end

local function set_listener(window, id, model)
	local callback = ui_callback[id]
	function callback.set(k,v)
		model[k] = v
	end
	function callback.ctrl(cmd)
		window[cmd]()
	end
	if not init_callback then
		init_callback = true
		set_callback(window, "set")
		set_callback(window, "ctrl")
		window.onMessage("query", function(name)
			return ui_meta[name]
		end)
		window.sendMessage("init_callback")
	end
end

local window_id = 0
local function alloc_id()
	window_id = window_id + 1
	ui_callback[window_id] = {}
	return window_id
end

function M.listen(window)
	return function (list)
		local window_id = alloc_id()
		local name = window.getName()
		local view = {}
		for _, item in ipairs(list) do
			view[item] = ""
		end
		local model = window.createModel(view)
		ui_meta[name] = {
			id = window_id,
			list = list,
		}
		set_listener(window, window_id, model)
	end
end

local game_side = {}
local ui_init

function M.init(ant)
	ant.gui_listen("init_callback", function()
		ui_init = true
	end)
end

local function init(name)
	local model = {}
	local obj = {
		name = name,
		model = model,
	}
	game_side[name] = obj
	return obj
end

function M.model(ant, name)
	local obj = game_side[name]
	if not obj then
		obj = init(name)
	end
	return obj.model
end

function M.show(ant, name, enable)
	local obj = game_side[name]
	if not obj then
		obj = init(name)
	end
	obj.show = enable
end

local function show(ant, obj)
	if not obj.id then
		return
	end
	local command = obj.show and "show" or "hide"
	ant.gui_send("ctrl", obj.id, command)
	obj.show = nil
end

local function query(ant, obj)
	if not ui_init then
		return
	end
	obj.id = false
	ltask.fork(function()
		local fullname = "/ui/"..obj.name..".html"
		local meta = ant.gui_call("query", fullname)
		obj.id = meta.id
		local remote = {}
		obj.remote = remote
		for _, name in ipairs(meta.list) do
			remote[name] = false
		end
	end)
end

function M.update(ant)
	for name, obj in pairs(game_side) do
		if not obj.open then
			local fullname = "/ui/"..name..".html"
			obj.open = true
			ant.gui_open(fullname)
		end
		if obj.show ~= nil then
			show(ant, obj)
		end
		local id = obj.id
		if id == nil then
			query(ant, obj)
		else
			local remote = obj.remote
			local model = obj.model
			for k,v in pairs(model) do
				local rv = remote[k]
				if rv ~= v and rv ~= nil then
					remote[k] = v
					model[k] = nil
					ant.gui_send("set", id, k, v)
				end
			end
		end
	end
end

return M