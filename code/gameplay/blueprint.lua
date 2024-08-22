return function (scene)
	local blueprint = {}
	local all = {}
	local add_queue = {}
	
	function blueprint.add(building, id, x, y)
		add_queue[#add_queue+1] = { what = "blueprint", action = "add", type = building, id = id, x = x, y = y }
		return id
	end

	function blueprint.del(id)
		add_queue[#add_queue+1] = { what = "blueprint", action = "del", id = id}
	end
	
	function blueprint.info(id, text)
		add_queue[#add_queue+1] = { what = "blueprint", action = "info", id = id, text = text}
	end

	function blueprint.update(message)
		local n = #message
		for k,v in ipairs(add_queue) do
			n = n + 1
			message[n] = v
			add_queue[k] = nil
		end
	end
	
	function blueprint.clear()
		all = {}
	end
	
	return blueprint
end
