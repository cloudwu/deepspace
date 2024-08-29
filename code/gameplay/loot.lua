return function (inst)
	local container = assert(inst.container)

	local loot = {}
	local last = {}	
	
	function loot.update(message)
		local n = #message
		local current = container.list_loot()
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
