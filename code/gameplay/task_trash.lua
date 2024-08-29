local worker_move = require "gameplay.worker_move"

return function (env)
	local move = worker_move.move
	function env:find_space()
		local container = self.context.container
		local stock, type = container.pile_stock(self.worker.cargo)
		if not stock then
			return self.cancel, "NO_CARGO"
		end
		local storage_list = container.list_storage()
		local x = self.worker.x
		local y = self.worker.y
		local result
		while true do
			local near = self.context.scene.nearest(x, y, storage_list)
			if not near then
				return self.cancel, "NO_SPACE"
			end
			local v = storage_list[near]
			local storage_id = v >> 32
			local n = container.storage_put(storage_id, type, stock)
			if n == 0 then
				-- no space in storage_id
				table.remove(storage_list, near)
			else
				container.storage_take(storage_id, type, n)
				self.space = storage_id
				result = v
				break
			end
		end

		local storage_x = (result >> 16) & 0xffff
		local storage_y = result & 0xffff
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
		return self.cont
	end
	function env:moveto_storage()
		local container = self.context.container
		local stock = container.storage_stock(self.space)
		if not stock then
			-- disappear
			return "find_space"
		end
		return move(self)
	end
	function env:put_cargo()
		local container = self.context.container
		local cargo = self.worker.cargo
		local stock, type = container.pile_stock(cargo)
		local n = container.storage_put(self.space, type, stock)
		container.pile_take(cargo, type, n)
		if stock == n then
			return self.cont
		end
		return "find_space"
	end
end
