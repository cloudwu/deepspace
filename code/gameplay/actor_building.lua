local datasheet = require "gameplay.datasheet"

return function (inst)
	local scene = inst.scene
	local blueprint = inst.blueprint
	local schedule = inst.schedule
	local container = inst.container
	local machine = inst.machine

	local building = {}
	
	local function complete_current_job(self)
		local complete = true
		local projects = self.project
		local live_projects = schedule.list()
		for p in pairs(projects) do
			if live_projects[p] then
				complete = false
			else
				projects[p] = nil
			end
		end
		return complete
	end
	
	local function build(self)
		self.status = "building"
		local p = schedule.new {
			type = "building",
			x = self.x,
			y = self.y,
			blueprint = self.blueprint,
			near = self.near,
		}
		self.project[p] = true
	end
	
	function building:update()
		local status = self.status
		if status == "invalid" then
			return true
		end
		if not complete_current_job(self) then
			return
		end
		if status == "blueprint" then
			build(self)
		elseif status == "building" then
			-- complete building task
			if self.pile then
				container.del_pile(self.pile)
				self.pile = nil
			end
			blueprint.del(self.id)
			machine.del(self.blueprint)
			scene.add_building(self.building, self.x, self.y)
			return true
		end
	end
	
	function building:init()
		local x = assert(self.x)
		local y = assert(self.y)
		local status = self.status

		if status == nil then
			self.status = "blueprint"
			-- publish project
			local building_id = assert(self.building)
			local building_data = assert(datasheet.building[building_id])
			
			blueprint.add(building_id, self.id, x, y)
			self.blueprint = machine.add(building_id)
			
			local project = {}
			self.project = project
			
			if building_data.material then
				local pile_id = container.add_pile()
				self.pile = pile_id

				for _, mat in ipairs(building_data.material) do
					local p = schedule.new {
						type = "supply",
						x = x,
						y = y,
						material = mat.id,
						count = mat.count,
						owner = self.id,
						pile = pile_id,
						near = self.near,
					}
					project[p] = true
				end
			end
		else
			-- load
			assert (status == "blueprint" or status == "building")
			local building_id = assert(self.building)
			blueprint.add(building_id, self.id, x, y)
			machine.add(building_id, self.blueprint)	-- bind buidling_id to self.blueprint
				
			local project = {}
			self.project = project
			local live_projects = schedule.list()
			for id, p in pairs(live_projects) do
				if p.owner == self.id or p.blueprint == self.blueprint then
					project[id] = true
				end
			end
		end
	end
	
	function building:debug()
		local status = self.status
		if status == "blueprint" then
			if self.pile then
				blueprint.info(self.id, container.pile_info(self.pile))
			end
		else	-- "building"
			blueprint.info(self.id, machine.info(self.blueprint))
		end
	end
	
	function building:export(list)
		local obj = {
			name = self.name,
			id = self.id,
			x = self.x,
			y = self.y,
			building = self.building,
			pile = self.pile,
			status = self.status,
			blueprint = self.blueprint,
			near = self.near,
		}
		list[#list+1] = obj
	end
	
	return building
end

