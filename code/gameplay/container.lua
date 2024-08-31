local C = require "game.container"
local datasheet = require "gameplay.datasheet"

return function()
	local container = {}
	local storage = C.new(datasheet.material_stack)	-- warehouse
	local pile = C.new(datasheet.material_stack) -- buildplan / Inventory / input / output box , etc
	local loot = C.new(datasheet.material_stack)
	local pos = {}
	local loot_pos = {}
	local loot_lookup = {}
	
	function find_(cdata, pos_index)
		local find_result = {}
		return function (type)
			local n = cdata:find(type, find_result)
			if n == 0 then
				return
			end
			for i = 1, #find_result do
				local id = find_result[i]
				find_result[i] = id << 32 | pos_index[id]
			end
			return find_result
		end
	end
	
	function list_(cdata, pos_index)
		local find_result = {}
		return function ()
			cdata:list(find_result)
			local n = #find_result
			if n == 0 then
				return
			end
			for i = 1, n do
				local id = find_result[i]
				find_result[i] = id << 32 | pos_index[id]
			end
			return find_result
		end
	end
	
	container.list_loot = list_(loot, loot_pos)
	
	function container.list_loot_pile()
		local r = {}
		for index, pile_id in pairs(loot_lookup) do
			local c = loot:content(pile_id)
			for type, count in pairs(c) do
				r[type << 32 | index] = count
			end
		end
		return r
	end
	
	function container.add_loot(x, y, what, count)
		local pos_index = x << 16 | y
		local pile_id = loot_lookup[pos_index]
		if not pile_id then
			pile_id = loot:add()
			loot_lookup[pos_index] = pile_id
			loot_pos[pile_id] = pos_index
		end
		loot:put(pile_id, what, count)
		return pile_id
	end
	
	function container.get_loot(x, y)
		local pos_index = x << 16 | y
		return loot_lookup[pos_index]
	end
	
	function container.empty_loot(x, y)
		local pos_index = x << 16 | y
		local pile_id = loot_lookup[pos_index]
		if pile_id then
			local c = loot:stock(pile_id)
			if not c then
				loot:remove(pile_id)
				loot_lookup[pos_index] = nil
				loot_pos[pile_id] = nil
				return true
			else
				return false
			end
		end
		return true
	end
	
	function container.take_loot(id, what, count)
		return loot:take(id, what, count)
	end
	
	function container.check_loot(id, what)
		return loot:stock(id, what)
	end
	
	container.find_loot = find_(loot, loot_pos)
	
	function container.add_storage(x, y, size)
		local id = storage:add(size)
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

	container.find_storage = find_(storage, pos)
	container.list_storage = list_(storage, pos)

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
		local temp = loot:export()
		if next(temp) then
			for _, pile in ipairs(temp) do
				local index = loot_pos[pile.id]
				pile.x = index >> 16
				pile.y = index & 0xffff
			end
			file:write_list("loot", temp)
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
		local obj = savedata.loot
		if obj then
			loot:import(obj)
			for _, pile in ipairs(obj) do
				local index = pile.x << 16 | pile.y
				loot_lookup[index] = pile.id
				loot_pos[pile.id] = index
			end
		end
	end
	
	local function clear(t)
		for k in pairs(t) do
			t[k] = nil
		end
	end
	
	function container.clear()
		storage:import()
		pile:import()
		clear(pos)
		clear(loot_pos)
		clear(loot_lookup)
	end
	
	return container
end