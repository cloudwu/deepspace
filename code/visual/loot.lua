local datasheet = require "gameplay.datasheet"

local loot = {}

local all = {}

function loot.update()
	for _, set in pairs(all) do
		for _,obj in pairs(set) do
			obj.r = obj.r + 1
		end
	end
end

local arrange = {}

arrange[1] = function (set, x, y)
	for k, obj in pairs(set) do
		obj.x = x
		obj.y = y
		return
	end
end

arrange[2] = function (set, x, y)
	x = x - 0.3
	for k, obj in pairs(set) do
		obj.x = x
		obj.y = y
		x = x + 0.6
	end
end

arrange[3] = function (set, x, y)
	y = y - 0.3
	local flag
	for k, obj in pairs(set) do
		obj.x = x
		obj.y = y
		if not flag then
			flag = true
			x = x - 0.3
			y = y + 0.6
		else
			x = x + 0.6
		end
	end
end

arrange[4] = function (set, x, y)
	x = x - 0.3
	y = y - 0.3
	local flag
	for k, obj in pairs(set) do
		obj.x = x
		obj.y = y
		x = x + 0.6
		if not flag then
			flag = true
			y = y + 0.6
		end
	end
end

local function count(v)
	local n = 0
	for k in pairs(v) do
		n = n + 1
	end
	return n
end

local function new_object(set, typeid, x, y, size)
	local n = count(set)
	if n >= 4 then
		return
	end
	local mat = datasheet.material[typeid].facade
	local obj = {
		z = 0.5,
		r = 0,
		s = size,
		material = {
			color = mat.color,
		},
	}
	set[typeid] = obj
	arrange[n+1](set, x, y)
	ant.primitive(mat.primitive, obj)
end

local function remove_object(set, typeid, x, y)
	local obj = set[typeid]
	if obj then
		ant.remove(obj)
		set[typeid] = nil
		if not next(set) then
			return			
		end
		arrange[count(set)](set, x, y)
	end
end

function loot.change(msg)
	local index = msg.x << 16 | msg.y
	local v = all[index]
	if not v then
		v = {}
		all[index] = v
	end
	local typeid = msg.type
	local size = msg.count
	if size == 0 then
		remove_object(v, typeid, msg.x, msg.y)
		return
	end
	size = math.log(size, 10) * 0.1 + 0.001
	local obj = v[typeid]
	if obj then
		obj.s = size
	else
		new_object(v, typeid, msg.x, msg.y, size)
	end
end

function loot.clear()
	for _,set in pairs(all) do
		for _,obj in pairs(set) do
			ant.remove(obj)
		end
	end
	all = {}
end

return loot
