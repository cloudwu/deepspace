local ui = import_package "rogue.ui"

local hud = {}

local mode
local events = {}

local function add_event(...)
	events[#events+1] = { ... }
end

local function map_coord(x, y)
	x, y = ant.camera_ctrl.screen_to_world(x, y)
	if x then
		x = (x + 0.5) // 1 | 0
		y = (y + 0.5) // 1 | 0
		return x, y
	end
end

local draging_x, draging_y
local draging_tx, draging_ty

ant.gesture_listen("tap", function (ev)
	if mode == "build" or mode == "remove" or mode == "deck" then
		local x, y = map_coord(ev.x, ev.y)
		if x then
			add_event(mode, true, x, y, x, y)
		end
	elseif mode == "worker" or mode == "box" or mode == "blueprint" or mode == "wood" or mode == "iron" or mode == "furnace" then
		local x, y = map_coord(ev.x, ev.y)
		if x then
			add_event(mode, "tap", x, y)
		end
	end
end)

ant.gesture_listen("pan", function (ev)
	if mode == "build" or mode == "remove" or mode == "deck" then
		if ev.state == "began" then
			local x, y = map_coord(ev.x, ev.y)
			if x then
				add_event(mode, false, x, y, x, y)
				draging_x = x
				draging_y = y
				draging_tx = x
				darging_ty = y
			end
		elseif ev.state == "changed" then
			local x, y = map_coord(ev.x, ev.y)
			if x then
				if not draging_x then
					draging_x = x
					draging_y = y
				end
				if draging_tx ~= x or darging_ty ~= y then
					draging_tx = x
					darging_ty = y
					add_event(mode, false, draging_x, draging_y, x, y)
				end
			end
		elseif ev.state == "ended" then
			local x, y = map_coord(ev.x, ev.y)
			if x then
				draging_tx = x
				draging_ty = y
			end
			if draging_x then
				add_event(mode, true, draging_x, draging_y, draging_tx, draging_ty)
			end
			draging_x = nil
			draging_y = nil
			draging_tx = nil
			darging_ty = nil
		end
	end
end)

function hud.init()
	ant.gui_open "/ui/hud.html"
	ant.gui_listen("mode", function(arg)
		if mode ~= arg then
			add_event("change_mode", arg)
			mode = arg
		end
	end)
	ui.init(ant)
end

function hud.update()
	ui.update(ant)
end

function hud.show(name, enable)
	ui.show(ant, name, enable)
end

function hud.model(name)
	return ui.model(ant, name)
end

local function next_values(t, key)
	if key == nil then
		key = 1
	else
		key = key + 1
	end
	local v = t[key]
	if v == nil then
		return
	else
		return key, table.unpack(v)
	end
end

function hud.message()
	local r = events
	events = {}
	return next_values, r
end

function hud.mode()
	return mode
end

local right_click_cancel
function hud.mouse_message(btn, state, x, y)
	if btn == "RIGHT" then
		if state == "DOWN" then
			right_click_cancel = nil
		elseif state == "MOVE" then
			right_click_cancel = true
		elseif state == "UP" then
			if right_click == nil then
				if not right_click_cancel then
					ant.gui_send "cancel"
					mode = nil
				end
			end
		end			
	end
end

return hud
