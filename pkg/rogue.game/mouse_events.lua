local ecs = ...
local world = ecs.world

local math3d = require "math3d"
local monitor = require "monitor"

local M = {}

local camera = ecs.require "camera_ctrl"
local mouse_mb	= world:sub { "mouse" }

local function map_coord(x, y)
	local p = camera.screen_to_world(x, y)
	if p then
		local pos = math3d.tovalue(p)
		x, y = monitor.get_coord(pos[1], pos[3])
		x = (x + 0.5) // 1 | 0
		y = (y + 0.5) // 1 | 0
		return x, y
	end
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

local start_x, start_y, cur_x, cur_y
local moved
local draging

function M.mouse_events()
	local events = {}
	for _, btn, state, x, y in mouse_mb:unpack() do
		if btn == "LEFT" then
			x, y = map_coord(x, y)
			if state == "DOWN" then
				start_x = x or start_x
				start_y = y or start_y
				cur_x, cur_y = start_x, start_y
				if start_x then
					events[#events+1] = { "drag_begin", start_x, cur_x, start_y, cur_y }
				end
				draging = true
			elseif state == "MOVE" then
				cur_x = x or cur_x
				cur_y = y or cur_y
				if not start_x then
					events[#events+1] = { "drag_begin", start_x, cur_x, start_y, cur_y }
					start_x, start_y = cur_x, cur_y
				end
				local n = #events
				local ev = events[n]
				if ev == nil or ev[1] ~= "draging" then
					n = n + 1
				end
				events[n] = { "draging", start_x, cur_x, start_y, cur_y }
			else
				-- state == "UP"
				events[#events+1] = { "drag_end", start_x, cur_x, start_y, cur_y }
				if start_x == cur_x and start_y == cur_y then
					events.leftclick = { start_x, cur_x }
				end
				start_x = nil
				start_y = nil
				cur_x = nil
				cur_y = nil
				draging = false
			end
		elseif btn == "RIGHT" then
			if state == "MOVE" then
				moved = true
			elseif state == "UP" then
				events[#events+1] = { "rightclick", moved, draging }
				moved = false
			end
		end
	end
	if not draging then
		local mouse = world:get_mouse()
		local x, y = map_coord(mouse.x, mouse.y)
		events[#events+1] = { "hover", x, y }
	end
	
	return next_values, events
end

return M