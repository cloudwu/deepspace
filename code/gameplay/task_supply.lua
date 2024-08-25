local worker_move = require "gameplay.worker_move"

return function (env)
	local move = worker_move.move
	function env:find_material()
		local container = self.context.container
		local stock, type = container.pile_stock(self.worker.cargo)
		local material = self.task.material
		local need = self.task.count
		if stock then
			if material ~= type then
				-- todo: return inventory to storage
				-- workaround : drop it
				container.pile_take(self.worker.cargo, type, stock)
				return self.cancel, "WRONG_CARGO"
			end
			if stock >= need then
				-- enough stock
				return "check_destination"
			end
		end
		local storage_list = container.find_storage(material)
		local x = self.worker.x
		local y = self.worker.y
		local near = self.context.scene.nearest(x, y, storage_list)
		if not near then
			if stock then
				-- have part of material
				return "check_destination"
			end
			return self.cancel, "NO_CARGO"
		end
		local v = storage_list[near]
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
		return self.cont
	end
	function env:moveto_material()
		local container = self.context.container
		local stock = container.storage_stock(self.storage_id, self.task.material)
		if not stock then
			return "find_material"			
		end
		return move(self)
	end
	function env:take_material()
		local container = self.context.container
		local stock, type = container.pile_stock(self.worker.cargo)
		local material = self.task.material
		local need = self.task.count
		if stock then
			assert(type == material)
			if stock >= need then
				return self.cont
			end
			need = need - stock
		end
		local take_count = container.storage_take(self.storage_id, material, need)
		container.pile_put(self.worker.cargo, material, take_count)
		if take_count ~= need then
			return "find_material"
		end
		return self.cont
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
