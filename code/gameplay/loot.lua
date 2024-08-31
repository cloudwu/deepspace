return function (inst)
	local container = assert(inst.container)

	local loot = {}
	local last = {}	
	local drop = {}
	
	function loot.drop(x, y, type, count)
		local index = x << 16 | y
		local c = drop[index]
		if not c then
			c = {
				name = "loot",
				x = x,
				y = y,
				content = {}
			}
			drop[index] = c
		end
		c.content[type] = (c.content[type] or 0) + count
	end
	
	function loot.update(message)
		if next(drop) then
			for k,v in pairs(drop) do
				drop[k] = nil
				inst.actor.new(v)
			end
		end
		local n = #message
		local current = container.list_loot_pile()
		for index, count in pairs(current) do
			local last_count = last[index]
			if last_count ~= count then
				-- count changes
				local what = index >> 32
				local x = (index >> 16) & 0xffff
				local y = index & 0xffff
				n = n + 1
				message[n] = { what = "loot", action = "change", type = what, x = x, y = y, count = count }
			end
			last[index] = nil
		end
		for index, count in pairs(last) do
			local what = index >> 32
			local x = (index >> 16) & 0xffff
			local y = index & 0xffff
			n = n + 1
			message[n] = { what = "loot", action = "change", type = what, x = x, y = y, count = 0 }
		end
		last = current
	end
	
	function loot.clear()
		last = {}
	end
	
	return loot
end
