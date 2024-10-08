local M = {}

-- todo: set speed
local speed = ant.setting.speed / ant.setting.fps

function M:move()
	local obj = self.worker.object
	local worker = self.worker
	local x = self.to_x
	local y = self.to_y
	local step = self.step
	if step == 0 then
		obj.x = x
		obj.y = y
		worker.x = (x + 0.5) // 1 | 0
		worker.y = (y + 0.5) // 1 | 0
		local path = self.path
		local n = #path
		if n == 0 then
			-- arrive
			self.to_x = nil
			self.to_y = nil
			return self.cont
		else
			local next_x = path[n-1]
			local next_y = path[n]
			path[n-1] = nil
			path[n] = nil
			self.to_x = next_x
			self.to_y = next_y
			local dx = next_x - x
			local dy = next_y - y
			local d = (dx * dx + dy * dy) ^ 0.5
			step = ((d / speed) + 0.5) // 1 | 0
			self.step = step
			if step > 0 then
				self.dx = dx / step
				self.dy = dy / step
			end
			return self.yield
		end
	end

	self.step = step - 1
	local x = obj.x + self.dx
	local y = obj.y + self.dy
	obj.x = x
	obj.y = y
	worker.x = ( x + 0.5 ) // 1 | 0
	worker.y = ( y + 0.5 ) // 1 | 0
	return self.yield
end

function M:walk_to_destination()
	local path = self.path or {}
	self.path = path
	path.x = self.worker.x
	path.y = self.worker.y
	local task = self.task
	local dist = self.context.scene.path(path, task.x, task.y, task.near)
	if dist == 0 then
		-- todo: drop material
		return self.cancel, "NO_PATH"
	end
	self.step = 0
	local obj = self.worker.object
	self.to_x = obj.x
	self.to_y = obj.y
	return self.cont
end

return M