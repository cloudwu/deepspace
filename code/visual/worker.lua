local worker = {}

local workers = {}

function worker.new(arg)
	workers[ant.sprite2d("/asset/avatar.texture", arg.object)] = true
end

function worker.update()
	local r = ant.camera_ctrl.yaw
	
	for obj in pairs(workers) do
		obj.r = r
		if obj.text then
			ant.print(obj, obj.text)
		end
	end
end

function worker.clear()
	for obj in pairs(workers) do
		ant.remove(obj)
	end
	workers = {}
end


return worker
