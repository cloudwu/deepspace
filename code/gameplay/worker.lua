return function (scene)
	local worker = {}
	
	local all = {}

	local add_queue = {}
	function worker.add(worker_id, x, y)
		if scene.valid(x, y) then
			local obj = { x = x, y = y }
			all[worker_id] = obj
			add_queue[#add_queue+1] = { what = "worker", action = "new", object = obj }
			return obj
		end
	end
	
	function worker.remove(worker_id)
		local obj = assert(all[worker_id])
		all[worker_id] = nil
		add_queue[#add_queue+1] = { what = "worker", action = "delete", object = obj }
	end
	
	function worker.update(message)
		local n = #message
		for k,v in ipairs(add_queue) do
			n = n + 1
			message[n] = v
			add_queue[k] = nil
		end
	end
	
	return worker
end
