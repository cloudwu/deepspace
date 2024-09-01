local datasheet = require "gameplay.datasheet"

local blueprint = {}

local all = {}

function blueprint.add(arg)
	local b = datasheet.building[arg.building].blueprint
	local obj = {
		x = arg.x,
		y = arg.y,
		s = b.s,
		z = b.z,
		material = {
			color = b.color,
		}
	}
	if b.primitive then
		ant.primitive(b.primitive, obj)
	else
		local name = "/asset/" .. b.prefab .. "/trans.prefab"
		obj.material.color = 0x80808080
		ant.prefab(name, obj)
	end
	all[arg.id] = obj
end

function blueprint.del(arg)
	local obj = all[arg.id] or error ("No blueprint " .. arg.id)
	ant.remove(obj)
	all[arg.id] = nil
end

function blueprint.info(arg)
	local obj = all[arg.id]
	if obj then
		ant.print(obj, arg.text)
	end
end

function blueprint.clear()
	for id, obj in pairs(all) do
		ant.remove(obj)
	end
	all = {}
end

return blueprint
