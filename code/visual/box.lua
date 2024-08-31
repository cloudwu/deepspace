local datasheet = require "gameplay.datasheet"

local box = {}

local all = {}

function box.new(arg)
	local f = datasheet.building[arg.building].facade
	-- todo : support prefab
	
	local obj = arg.object
	obj.s = f.scale
	obj.z = 0.5
	obj.material = {
		color = f.color,
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

function box.update()
	for _, obj in pairs(all) do
		if obj.text then
			ant.print(obj, obj.text)
		end
	end
end

return box
