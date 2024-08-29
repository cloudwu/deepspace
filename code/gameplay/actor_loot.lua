return function (inst)
	local schedule = inst.schedule
	local container = inst.container
	local scene = inst.scene

	local loot = {}
	
	function loot:update()
		local pile = self.pile
		if not pile then
			-- quit
			return true
		end
		if self.project then
			local live_projects = schedule.list()
			if not live_projects[self.project] then
				self.project = nil
			else
				-- continue
				return
			end
		end
		
		if container.empty_loot(self.x, self.y) then
			return true
		end
		
		local content = container.pile_content(pile)
		local storage_list = container.list_storage()
		if storage_list and scene.reachable(self.x, self.y, storage_list) then
			self.project = schedule.new {
				type = "cleanup",
				x = self.x,
				y = self.y,
			}
		end
	end
	
	function loot:init()
		local x = assert(self.x)
		local y = assert(self.y)
		local pile_id = self.pile

		if pile_id == nil then
			local pile_id = container.pile_loot(x, y)
			for what, count in pairs(self.content) do
				container.add_loot(x, y, what, count)
			end
			if pile_id then
				-- pile already exist
				return
			end
			self.content = nil
			self.pile = assert(container.pile_loot(x, y))
		else
			-- todo: support more concurrent project
			local live_projects = schedule.list()
			for id, p in pairs(live_projects) do
				if p.owner == self.id then
					self.project = id
					break
				end
			end
		end
	end
	
	function loot:debug()
		--	todo: set loot info
	end
	
	function loot:export(list)
		if not self.pile then
			return
		end
		local obj = {
			name = self.name,
			id = self.id,
			x = self.x,
			y = self.y,
			pile = self.pile,
		}
		list[#list+1] = obj
	end
	
	return loot
end

