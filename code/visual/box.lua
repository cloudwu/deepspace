local datasheet = require "gameplay.datasheet"

local box = {}

local all = {}

function box.new(arg)
	local f = datasheet.building[arg.building].facade
	-- todo : support prefab
	
	local obj = {
		x = arg.x,
		y = arg.y,
		s = f.scale,
		z = 0.5,
		material = {
			color = f.color,
		}
	}
	assert(all[arg.id] == nil)
	all[arg.id] = ant.primitive("cube", obj)
end

function box.del(arg)
	ant.remove(all[arg.id])
	all[arg.id] = nil
end

function box.clear()
	for _, obj in pairs(all) do
		ant.remove(obj)
	end
	all = {}
end

function box.info(arg)
	local obj = all[arg.id]
	if obj then
		ant.print(obj, arg.text)
	end
end

return box
