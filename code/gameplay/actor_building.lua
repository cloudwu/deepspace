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
		blueprint.del(self.blueprint)
		return true
	end
	
	function building:init()
		if self.status == nil then
			self.status = "blueprint"
			-- publish project
			local x = assert(self.x)
			local y = assert(self.y)
			
			if not scene.valid(x, y) then
				self.status = "invalid"
				return
			end

			local building_id = assert(self.building)
			local building_data = assert(datasheet.building[building_id])
			self.blueprint = blueprint.add(building_id, x, y)
			
			local project = {}
			self.project = project
			
			for _, mat in ipairs(building_data.material) do
				local p = schedule.new {
					type = "supply",
					x = x,
					y = y,
					material = mat.id,
					count = mat.count,
				}
				project[p] = true
			end
		end
	end
	
	return building
end

