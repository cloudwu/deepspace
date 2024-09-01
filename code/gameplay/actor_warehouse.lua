local datasheet = require "gameplay.datasheet"
local debuginfo = require "gameplay.debuginfo"

return function (inst)
	local scene = inst.scene
	local schedule = inst.schedule
	local container = inst.container

	local warehouse = {}
	
	function warehouse:update()
		if self.project then
			local live_projects = schedule.list()
			if not live_projects[self.project] then
				self.project = nil
			else
				-- continue
				return
			end		
		end
		local loot_list = container.list_loot()
		local near = scene.nearest(self.x, self.y, loot_list)
		if near then
			local v = loot_list[near]
			local x = v >> 16 & 0xffff
			local y = v & 0xffff
			self.project = schedule.new {
				type = "loot",
				x = x,
				y = y,
				owner = self.id,
			}
		end
	end
	
	function warehouse:init()
		local x = assert(self.x)
		local y = assert(self.y)
		
		if not self.storage then
			local size = datasheet.building[self.type].size or error (self.type .. " need .size")
			self.storage = container.add_storage(x, y, size)
		else
			-- loading
			-- todo: support more than one worker
			local live_projects = schedule.list()
			for id, p in pairs(live_projects) do
				if p.owner == self.id then
					self.project = id
				end
			end
		end
		return { what = "box", action = "new", id = self.id, building = self.type, x = x, y = y }
	end
	
	function warehouse:deinit()
		container.del_storage(self.storage)
		return { what = "box", action = "del", id = self.id }
	end
	
	function warehouse:debug()
		debuginfo.add("box", self.id, container.storage_info(self.storage))
	end

	function warehouse:export(list)
		local obj = {
			name = self.name,
			id = self.id,
			type = self.type,
			x = self.x,
			y = self.y,
			storage = self.storage,
		}
		list[#list+1] = obj
	end
	
	return warehouse
end

