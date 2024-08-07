local task = require "gameplay.task"
local util = require "util"

local task_temp = util.map_from_list({
	"supply",
}, function (name)
	return task.define(require ("gameplay.task_" .. name))
end)

local function get_task(scene, x, y, projects)
	local min_dist
	local best_id
	for id, task in pairs(projects) do
		if task.worker == nil then
			local dist = scene.dist(x, y, task.x, task.y)
			if dist > 0	then
				if not min_dist or dist < min_dist then
					min_dist = dist
					best_id = id
				end
			end
		end
	end
	return best_id, min_dist
end

return function (inst)
	local schedule = inst.schedule
	local scene = inst.scene

	local worker = {}
	
	local status = {}
	
	function status:idle()
		local x, y = self.x, self.y
		local dist = scene.storage_dist(x, y)
		if dist == 0 then
			-- can't get material
			return
		end
		local task, min_dist = get_task(scene, x, y, schedule.list())
		if task then
			self.status = "wait_task"
			-- todo: priority ~= 100 
			schedule.request(task, self.id, 1000 - min_dist)
		end
	end
	
	function status:supply()
		local cont, err = self.task:update()
		if err then
			schedule.cancel(self.task_id, self.id)
		elseif not cont then
			schedule.complete(self.task_id, self.id)
		else
			return
		end
		self.task_id = nil
		self.task = nil
		self.status = "idle"
	end

	function worker:map_change()
		if self.status == "supply" then
			self.task:reset()
		end
	end
	
	local function assign_task(self, task_id, cp)
		local task = schedule.list()[task_id]
		local task_type = task.type
		self.status = task_type
		self.task_id = task_id
		self.task = task_temp[task_type]:instance {
			context = inst,
			worker = self,
			task = task,
		}
		self.task:reset(cp)
	end
	
	function worker:assign_task(task_id)
		assert(self.status == "wait_task")
		assign_task(self, task_id)
	end
	
	function worker:reject_task(task_id)
		assert(self.status == "wait_task")
		self.status = "idle"
	end
	
	function worker:update()
		return status[self.status](self)
	end
	
	function worker:init()
		if self.status == nil then
			self.status = "idle"
			self.object = inst.worker.add(self.id, self.x, self.y)
		else
			local x = self.x
			local y = self.y
			local wx = (x + 0.5) // 1 | 0
			local wy = (y + 0.5) // 1 | 0
			self.x = wx
			self.y = wy
			self.object = inst.worker.add(self.id, wx, wy, x, y)
			if self.task_id then
				assign_task(self, self.task_id, self.task)
			end
		end
	end
	
	function worker:export(list)
		local obj = {
			name = self.name,
			id = self.id,
			x = self.object.x,
			y = self.object.y,
			status = self.status,
			task_id = self.task_id,
		}
		if self.task then
			obj.task = self.task:reset()
		end
		list[#list+1] = obj
	end
	
	return worker
end

