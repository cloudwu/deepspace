local worker_move = require "gameplay.worker_move"

local function set_storage(self, x, y, v)
	local storage_id = v >> 32
	local storage_x = (v >> 16) & 0xffff
	local storage_y = v & 0xffff
	local path = {
		x = x,
		y = y,
	}
	self.context.scene.path(path, storage_x, storage_y)
	self.path = path
	self.step = 0
	local obj = self.worker.object
	self.to_x = obj.x
	self.to_y = obj.y
	self.storage_id = storage_id
end

return function (env)
	local move = worker_move.move
	function env:find_material()
		local container = self.context.container
		local stock, type = container.pile_stock(self.worker.cargo)
		local material = self.task.material
		local need = self.task.count
		if stock then
			assert(material == type)
			if stock >= need then
				-- enough stock
				return "check_destination"
			end
		end
		local x = self.worker.x
		local y = self.worker.y
		local storage_list = container.find_storage(material)
		local loot_list = container.find_loot(material)

		local near_storage, dist_storage = self.context.scene.nearest(x, y, storage_list)
		local near_loot, dist_loot = self.context.scene.nearest(x, y, loot_list)
		
		if not near_storage and not near_loot then
			if stock then
				return "check_destination"
			else
				return self.cancel, "NO_CARGO"
			end
		end
		
		if dist_storage < dist_loot then
			local v = storage_list[near_storage]
			set_storage(self, x, y, v)
			return "moveto_storage"
		else
			local v = loot_list[near_loot]
			set_storage(self, x, y, v)
			return "moveto_loot"
		end
	end
	function env:moveto_storage()
		local container = self.context.container
		local stock = container.storage_stock(self.storage_id, self.task.material)
		if not stock then
			return "find_material"
		end
		return move(self)
	end
	function env:take_storage()
		local container = self.context.container
		local stock, type = container.pile_stock(self.worker.cargo)
		local material = self.task.material
		local need = self.task.count
		if stock then
			assert(type == material)
			if stock >= need then
				return "check_destination"
			end
			need = need - stock
		end
		local take_count = container.storage_take(self.storage_id, material, need)
		container.pile_put(self.worker.cargo, material, take_count)
		if take_count ~= need then
			return "find_material"
		end
		return "check_destination"
	end
	function env:moveto_loot()
		local container = self.context.container
		local stock = container.check_loot(self.storage_id, self.task.material)
		if not stock then
			return "find_material"
		end
		return move(self)
	end
	function env:take_loot()
		local container = self.context.container
		local stock, type = container.pile_stock(self.worker.cargo)
		local material = self.task.material
		local need = self.task.count
		if stock then
			assert(type == material)
			if stock >= need then
				return "check_destination"
			end
			need = need - stock
		end
		local take_count = container.take_loot(self.storage_id, material, need)
		container.pile_put(self.worker.cargo, material, take_count)
		if take_count ~= need then
			return "find_material"
		end
		return "check_destination"
	end
	function env:check_destination()
		self:checkpoint()
		return self.cont
	end
	env.walk_to_destination = worker_move.walk_to_destination
	env.moveto_blueprint = move
	function env:put_material()
		local container = self.context.container
		local material = self.task.material
		local count = self.task.count
		local stock = container.pile_take(self.worker.cargo, material, count)
		if stock then
			container.pile_put(self.task.pile, material, stock)
		else
			return self.cancel, "NO_CARGO"
		end
		return self.cont
	end
end
