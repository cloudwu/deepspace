local blueprint = {}

local all = {}
local info = {}

function blueprint.add(arg)
	local obj = { x = arg.x, y = arg.y }
	obj.s = 1
	obj.z = 0.1
	obj.material = {
		color = 0x00ff00,
	}
	all[arg.id] = ant.primitive("cube", obj)
end

function blueprint.del(arg)
	local obj = all[arg.id] or error ("No blueprint " .. arg.id)
	ant.remove(obj)
	all[arg.id] = nil
	info[arg.id] = nil
end

function blueprint.info(arg)
	info[arg.id] = arg.text
end

function blueprint.update()
	for id, text in pairs(info) do
		ant.print(all[id], text)
	end
end

function blueprint.clear()
	for id, obj in pairs(all) do
		ant.remove(obj)
	end
	all = {}
	info = {}
end

return blueprint
