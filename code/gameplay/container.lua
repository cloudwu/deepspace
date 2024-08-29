local C = require "game.container"
local datasheet = require "gameplay.datasheet"

return function()
	local container = {}
	local storage = C.new(datasheet.material_stack)	-- warehouse
	local pile = C.new(datasheet.material_stack) -- buildplan / Inventory / input/output box , etc
	
	local pos = {}
	
	local loot = {}	-- actor_loot should manager loots, and publish cleanup task
	
	function container.list_loot()
		local r = {}
		for index, pile_id in pairs(loot) do
			local c = pile:content(pile_id)
			for type, count in pairs(c) do
				r[type << 32 | index] = count
			end
		end
		return r
	end
	
	function container.add_loot(x, y, what, count)
		local pos_index = x << 16 | y
		local pile_id = loot[pos_index]
		if not pile_id then
			pile_id = container.add_pile()
			loot[pos_index] = pile_id
		end
		pile:put(pile_id, what, count)
		return pile_id
	end
	
	function container.pile_loot(x, y)
		local pos_index = x << 16 | y
		return loot[pos_index]
	end
	
	function container.empty_loot(x, y)
		local pos_index = x << 16 | y
		local pile_id = loot[pos_index]
		if pile_id then
			local c = pile:stock(pile_id)
			if not c then
				pile:remove(pile_id)
				loot[pos_index] = nil
				return true
			else
				return false
			end
		end
		return true
	end
	
	function container.add_storage(x, y, id)
		if not id then
			local bid = datasheet.building_id.storage
			id = storage:add(datasheet.building[bid].size)
		end
		pos[id] = x << 16 | y
		return id
	end
	
	function container.del_storage(id)
		if pos[id] then
			pos[id] = nil
			storage:remove(id)
		end
	end

	function container.storage_exist(id)
		return pos[id] ~= nil
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
	
	do
		local find_result = {}
		function container.list_storage()
			local n = storage:list(find_result)
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
	
	function container.pile_content(id)
		return pile:content(id)
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
		loot = {}
	end
	
	return container
end