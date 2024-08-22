local C = require "game.container"
local datasheet = require "gameplay.datasheet"

return function()
	local container = {}
	local storage = C.new(datasheet.material_stack)	-- storage / output box, etc
	local pile = C.new(datasheet.material_stack) -- buildplan / Inventory / input box , etc
	
	local pos = {}
	
	function container.add_storage(x, y)
		local bid = datasheet.building_id.storage
		local id = storage:add(datasheet.building[bid].size)
		pos[id] = x << 16 | y
		return id
	end
	
	do
		local find_result = {}
		function container.find_storage(type)
			local n = storage:find(type, find_result)
			if n == 0 then
				return
			end
			for i = 1, #find_result do
				local id = find_result[i]							
				find_result[i] = id << 32 | pos[id]
			end
			return find_result
		end
	end
	
	function container.add_pile()
		return pile:add()
	end
	
	function container.del_pile(id)
		pile:remove(id)
	end
	
	function container.pile_stock(id, what)
		return pile:stock(id, what)
	end
	
	function container.pile_put(id, what, count)
		return pile:put(id, what, count)
	end

	function container.pile_take(id, what, count)
		return pile:take(id, what, count)
	end
	
	function container.storage_stock(id, what)
		return storage:stock(id, what)
	end
	
	function container.storage_take(id, what, count)
		return storage:take(id, what, count)
	end
	
	function container.storage_put(id, what, count)
		return storage:put(id, what, count)
	end
	
	local function get_info(c, id)
		local t = c:content(id)
		local r = {}
		local n = 1
		for type, count in pairs(t) do
			r[n] = datasheet.material[type].name .. ":" .. count
			n = n + 1
		end
		return table.concat(r, " ")
	end
	
	function container.storage_info(id)
		return get_info(storage, id)
	end

	function container.pile_info(id)
		return get_info(pile, id)
	end
	
	function container.export(file)
		local temp = storage:export()
		if next(temp) then
			file:write_list("storage", temp)
		end
		local temp = pile:export()
		if next(temp) then
			file:write_list("pile", temp)
		end
	end

	function container.import(savedata)
		local obj = savedata.storage
		if obj then
			storage:import(obj)
		end
		local obj = savedata.pile
		if obj then
			pile:import(obj)
		end
	end
	
	function container.clear()
		storage:import()
		pile:import()
		pos = {}
	end
	
	return container
end