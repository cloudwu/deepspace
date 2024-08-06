return function (scene)
	local box = {}

	local add_queue = {}
	function box.add(x, y)
		if scene.valid(x, y) then
			add_queue[#add_queue+1] = { x = x, y = y }
			-- todo: add material type
			scene.storage(x, y, true)
		end
	end
	
	function box.update(message)
		local n = #message
		for k,v in ipairs(add_queue) do
			n = n + 1
			message[n] = { what = "box", action = "new", object = v }
			add_queue[k] = nil
		end
	end
	
	return box
end
