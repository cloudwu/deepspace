local datasheet = require "gameplay.datasheet"

return function (inst)
	local scene = inst.scene
	local blueprint = inst.blueprint
	local schedule = inst.schedule

	local building = {}
	
	function building:update()
		if self.status == "invalid" then
			return true
		end
		local not_complete = false
		local projects = self.project
		local live_projects = schedule.list()
		for p in pairs(projects) do
			if live_projects[p] then
				not_complete = true
			else
				projects[p] = nil
			end
		end
		if not_complete then
			return
		end
		blueprint.del(self.id)
		return true
	end
	
	function building:init()
		local x = assert(self.x)
		local y = assert(self.y)
		
		if self.status == nil then
			self.status = "blueprint"
			-- publish project
			
			if not scene.valid(x, y) then
				self.status = "invalid"
				return
			end

			local building_id = assert(self.building)
			local building_data = assert(datasheet.building[building_id])
			blueprint.add(building_id, self.id, x, y)
			
			local project = {}
			self.project = project
			
			for _, mat in ipairs(building_data.material) do
				local p = schedule.new {
					type = "supply",
					x = x,
					y = y,
					material = mat.id,
					count = mat.count,
					owner = self.id,
				}
				project[p] = true
			end
		else
			-- load
			assert(self.status == "blueprint")
			local building_id = assert(self.building)
			local building_data = assert(datasheet.building[building_id])
			blueprint.add(building_id, self.id, x, y)
			
			local project = {}
			self.project = project
			local live_projects = schedule.list()
			for id, p in pairs(live_projects) do
				if p.owner == self.id then
					project[id] = true
				end
			end
		end
	end
	
	function building:export(list)
		local obj = {
			name = self.name,
			id = self.id,
			x = self.x,
			y = self.y,
			building = self.building,
			status = self.status,
		}
		list[#list+1] = obj
	end
	
	return building
end

