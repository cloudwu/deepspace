return function (inst)
	local scene = assert(inst.scene)
	local container = assert(inst.container)

	local box = {}
	
	local all = {}

	local add_queue = {}
	
	-- todo: remove
	function box.add(x, y, cid)
		if scene.valid(x, y) then
			if cid == nil then
				cid = container.add_storage(x, y)
			end

			local b = { x = x, y = y, id = cid }
			all[cid] = b
			add_queue[#add_queue+1] = b
			return cid
		end
	end

	function box.put(id, what, count)
		container.storage_put(id, what, count)
	end
	
	function box.debug_text(id, text)
		all[id].text = text
	end
	
	function box.clear()
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
	
	function box.debug()
		for k,v in pairs(all) do
			v.text = container.storage_info(k)
		end
	end

	local save_key <const> = "box"
	
	function box.export(file)
		if not next(all) then
			return
		end
		local temp = {}
		for k,v in pairs(all) do
			temp[k] = { x = v.x, y = v.y, id = v.id }
		end
		file:write_list(save_key, temp)
	end

	function box.import(savedata)
		local obj = savedata[save_key]
		if obj then
			for _, item in ipairs(obj) do
				box.add(item.x, item.y, item.id)
			end
		end
	end
		
	return box
end
