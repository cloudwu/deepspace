local monitor = require "monitor"

local M = {
	width = 128,
	height = 128,
}

local floor_prefab = "/asset/wall/floor.glb/mesh.prefab"

local map = {}

local function create_floor(world, x, y, visible, emissive)
	local proxy = { x = x, y = y , material = { emissive = emissive, visible = visible } }
	world:create_instance {
		prefab = floor_prefab,
		on_ready = function (inst)
			local eid_list = inst.tag["*"]
			local eid = eid_list[1]
			proxy.eid = eid
			local mat = proxy.material
			proxy.material = monitor.material(world, eid_list)
			proxy.material.emissive = mat.emissive
			proxy.material.visible = mat.visible
			monitor.new(proxy)
		end,
	}
	return proxy
end

local function trigger(world, x, y, visible)
	if x < 0 or x >= M.width or y < 0 or y >=M.height then
		return
	end
	local idx = x << 32 | y
	if map[idx] then
		local obj = map[idx]
		obj.material.visible = visible
	else
		local proxy = create_floor(world, x, y, visible, 0)
		map[idx] = proxy
	end
end

local lastobj
local focus

function M.focus_clear()
	if not focus then
		return
	end
	if not focus.material.hide then
		focus.material.hide = true
	end
end

function M.focus(world, x, y, color)
	if not focus then
		focus = create_floor(world, x, y, true)
	end
	if not x then
		focus.material.hide = true
		return
	end
	if focus.material.hide then
		focus.material.hide = false
	end
	
	focus.x = x
	focus.y = y
	
	if lastobj then
		lastobj.material.hide = false
	end
	
	if x < 0 or x >= M.width or y < 0 or y >=M.height then
		focus.material.emissive = 0x400000
		lastobj = nil
	else
		local idx = x << 32 | y
		local obj = map[idx]
		if obj and obj.material.visible then
			obj.material.hide = true
			lastobj = obj
			focus.material.emissive = color
		else
			focus.material.emissive = color * 4
			lastobj = nil
		end
	end
end

local function clamp(x1, x2, range)
	if x1 > x2 then
		x1, x2 = x2, x1
	end
	if x1 > range or x2 < 0 then
		return
	end
	if x1 < 0 then
		x1 = 0
	end
	if x2 > range then
		x2 = range
	end
	return x1, x2
end

local drag_tile = {}

function M.drag_clear(world)
	for _, t in ipairs(drag_tile) do
		local x, y = t.x, t.y
		t.material.hide = true
		local obj_idx = x << 32 | y
		local obj = map[obj_idx]
		if obj then
			obj.material.hide = false
		end
	end
end

function M.drag_clamp(start_x, end_x, start_y, end_y)
	start_x, end_x = clamp(start_x, end_x, M.width)
	start_y, end_y = clamp(start_y, end_y, M.height)
	if not start_x or not start_y then
		return
	end
	return start_x, end_x, start_y, end_y
end

local function trigger_range(add, world, x1, x2, y1, y2)
	x1, x2, y1, y2 = M.drag_clamp(x1, x2, y1, y2)
	if x1 then
		for x = x1, x2 do
			for y = y1, y2 do
				trigger(world, x, y, add)
			end
		end
	end
end

function M.add(...)
	trigger_range(true, ...)
end

function M.del(...)
	trigger_range(false, ...)
end

function M.drag(world, start_x, end_x, start_y, end_y, color)
	start_x, end_x, start_y, end_y = M.drag_clamp(start_x, end_x, start_y, end_y)
	if not start_x then
		return
	end
	local n = (end_x - start_x + 1) * (end_y - start_y + 1)
	for i = #drag_tile + 1, n do
		drag_tile[i] = create_floor(world, 0, 0, true)
	end
	local idx = 1
	for x = start_x, end_x do
		for y = start_y, end_y do
			local t = drag_tile[idx]; idx = idx + 1
			local obj_idx = x << 32 | y
			local obj = map[obj_idx]
			if obj and obj.material.visible then
				obj.material.hide = true
				t.material.emissive = color
			else
				t.material.emissive = color * 4
			end
			t.x = x
			t.y = y
			t.material.hide = false
		end
	end
end

return M
