local worker = {}

local workers = {}

function worker.new(arg)
	local id = arg.id
	assert(workers[id] == nil)
	workers[id] = ant.sprite2d("/asset/avatar.texture", arg.object)
end

function worker.del(arg)
	local obj = workers[arg.id] or error ("No worker " .. arg.id)
	ant.remove(obj)
	workers[arg.id] = nil
end

function worker.update()
	local r = ant.camera_ctrl.yaw
	
	for _, obj in pairs(workers) do
		obj.r = r
		if obj.text then
			ant.print(obj, obj.text)
		end
	end
end

function worker.clear()
	for _, obj in pairs(workers) do
		ant.remove(obj)
	end
	workers = {}
end


return worker
