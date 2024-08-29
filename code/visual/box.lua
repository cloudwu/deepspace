local box = {}

local all = {}

function box.new(arg)
	local obj = arg.object
	obj.s = 0.8
	obj.z = 0.5
	obj.r = 0
	obj.material = {
		color = 0xff0000ff,
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
		if obj.text then
			ant.print(obj, obj.text)
		end
	end
end

return box
