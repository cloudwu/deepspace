return function (scene)
	local blueprint = {}
	local all = {}
	local alloc_id; do
		local id = 0
		function alloc_id()
			id = id + 1
			return id
		end
	end

	local add_queue = {}
	
	function blueprint.add(building, x, y)
		local id = alloc_id()
		add_queue[#add_queue+1] = { what = "blueprint", action = "add", type = building, id = id, x = x, y = y }
		return id
	end

	function blueprint.del(id)
		add_queue[#add_queue+1] = { what = "blueprint", action = "del", id = id}
	end

	function blueprint.update(message)
		local n = #message
		for k,v in ipairs(add_queue) do
			n = n + 1
			message[n] = v
			add_queue[k] = nil
		end
	end
	
	return blueprint
end
