return function (scene)
	local box = {}
	
	local all = {}

	local add_queue = {}
	
	-- todo: remove
	function box.add(x, y)
		if scene.valid(x, y) then
			local b = { x = x, y = y }
			all[#all+1] = b
			add_queue[#add_queue+1] = b
			-- todo: add material type
			scene.storage(x, y, true)
		end
	end
	
	function box.clear()
		for _, item in ipairs(all) do
			scene.storage(item.x, item.y, false)
		end
		all = {}
	end
	
	function box.update(message)
		local n = #message
		for k,v in ipairs(add_queue) do
			n = n + 1
			message[n] = { what = "box", action = "new", object = v }
			add_queue[k] = nil
		end
	end
	
	local save_key <const> = "box"
	
	function box.export(file)
		if not next(all) then
			return
		end
		local temp = {}
		for k,v in ipairs(all) do
			temp[k] = { x = v.x, y = v.y }
		end
		file:write_list(save_key, temp)
	end

	function box.import(savedata)
		local obj = savedata[save_key]
		if obj then
			for _, item in ipairs(obj) do
				box.add(item.x, item.y)
			end
		end
	end
		
	return box
end
