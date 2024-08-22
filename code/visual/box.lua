local box = {}

local all = {}

function box.new(arg)
	local obj = arg.object
	obj.s = 0.5
	obj.z = 0.5
	obj.r = 0
	obj.material = {
		color = 0x0000ff,
	}
	all[ant.primitive("cube", obj)] = true
end

function box.clear()
	for obj in pairs(all) do
		ant.remove(obj)
	end
	all = {}
end

function box.update()
	for obj in pairs(all) do
		obj.r = obj.r + 1
		if obj.text then
			ant.print(obj, obj.text)
		end
	end
end

return box
