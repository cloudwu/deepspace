local floor = {}

local cache = {}
local objs = {}
local focus = {}
local focus_obj = ant.prefab("/asset/wall/floor.glb/mesh.prefab", { x = 0, y = 0, z = 0.1 })

local function enable_floor(x, y)
	local index = x << 16 | y
	local obj = objs[index]
	if not obj then
		obj = table.remove(cache)
		if obj then
			obj.material.visible = true
			obj.material.color = 0xffffffff
			obj.x = x
			obj.y = y
			obj.z = 0
		else
			obj = ant.prefab("/asset/wall/floor.glb/mesh.prefab", { x = x, y = y, z = 0, material = { color = 0xffffff } })
		end
		objs[index] = obj
	end
end

local function color_floor(x, y, color)
	local index = x << 16 | y
	local obj = focus[index]
	if not obj then
		obj = table.remove(cache)
		if obj then
			obj.material.visible = true
			obj.material.color = color
			obj.x = x
			obj.y = y
			obj.z = 0.1
		else
			obj = ant.prefab("/asset/wall/floor.glb/mesh.prefab", { x = x, y = y, z = 0.1, material = { color = color }})
		end
		focus[index] = obj
	end
end

local function clear_focus()
	for k,obj in pairs(focus) do
		obj.material.visible = false
		table.insert(cache, obj)
		focus[k] = nil
	end
	focus_obj.material.visible = false
end

local function disable_floor(x, y)
	local index = x << 16 | y
	local obj = objs[index]
	if obj then
		obj.material.visible = false
		table.insert(cache, obj)
		objs[index] = nil
	end
end

function floor.build(x, y)
	enable_floor(x, y)
end

function floor.remove(x, y)
	disable_floor(x, y)
end

local mode_color = {
	build = { 0x5050f0 , 0x4040c0 },
	remove = { 0xf05050, 0xc04040 },
}


local function clamp(x, y)
	if x < 0 then
		x = 0
	elseif x > 0xffff then
		x = 0xffff
	end
	if y < 0 then
		y = 0
	elseif y > 0xffff then
		y = 0xffff
	end
	return x, y
end

local function range(x1, y1, x2, y2)
	x1, y1 = clamp(x1, y1)
	x2, y2 = clamp(x2, y2)
	if x1 > x2 then
		x1, x2 = x2, x1
	end
	if y1 > y2 then
		y1, y2 = y2, y1
	end
	return x1, y1, x2, y2
end

function floor.draging(mode, x1, y1, x2, y2)
	clear_focus()
	if mode then
		x1, y1, x2, y2 = range(x1, y1, x2, y2)
		for x = x1, x2 do
			for y = y1, y2 do
				local index = x << 16 | y
				local c = objs[index] and mode_color[mode][1] or mode_color[mode][2]
				color_floor(x, y, c)
			end
		end
	else
		focus_obj.material.visible = true
	end
end

function floor.debug()
	for index, obj in pairs(objs) do
		local x, y = index >> 16 , index & 0xffff
		ant.print(obj, ("(%d,%d)"):format(x,y))
	end
end

local focus_color = {
	build = 0x6060ff,
	remove = 0xff6060,
}

function floor.focus(x, y, mode)
	focus_obj.x = x
	focus_obj.y = y
	focus_obj.material.color = 	focus_color[mode] or 0x808020
end

function floor.clear()
	for index, obj in pairs(objs) do
		obj.material.visible = false
		table.insert(cache, obj)
		objs[index] = nil
	end
end

return floor