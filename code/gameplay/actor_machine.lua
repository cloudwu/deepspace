local datasheet = require "gameplay.datasheet"
local debuginfo = require "gameplay.debuginfo"

return function (inst)
	local scene = inst.scene
	local schedule = inst.schedule
	local container = inst.container
	local machine = inst.machine

	local M = {}
	
	local function input_consume(pile, desc)
		if not desc then
			return true
		end
		local enough = true
		for what, count in pairs(desc) do
			local n = container.pile_stock(pile, what)
			if not n or n < count then
				enough = false
				break
			end
		end
		if enough then
			for what, count in pairs(desc) do
				container.pile_take(pile, what, count)
			end
			return true
		end
	end
	
	local function output_take(pile, desc)
		if not desc then
			return true
		end
		for what, count in pairs(desc) do
			container.pile_put(pile, what, count)
		end
	end
	
	function M:update()
		local supply = {}
		local projects = self.project
		local live_projects = schedule.list()
		local operate_finish
		for p in pairs(projects) do
			local obj = live_projects[p]
			if not obj then
				projects[p] = nil
				if p == self.operate then
					operate_finish = true
					self.operate = nil
				elseif p == self.deliver then
					self.deliver = nil
				end
			else
				if obj.type == "supply" then
					supply[obj.material] = (supply[obj.material] or 0 ) + obj.count
				end
			end
		end
		if not self.deliver and container.pile_stock(self.output) then
			-- output
			local p = schedule.new {
				type = "deliver",
				x = self.x,
				y = self.y,
				owner = self.id,
				pile = self.output,
			}
			projects[p] = true
			self.deliver = p
		end
		if not self.recipe then
			return
		end
		local desc = datasheet.recipe[self.recipe]
		if desc.input then
			-- take 2x input
			for what, count in pairs(desc.input) do
				local need = count * 2 - (supply[what] or 0) - (container.pile_stock(self.input, what) or 0)
				if need > 0 then
					local p = schedule.new {
						type = "supply",
						x = self.x,
						y = self.y,
						material = what,
						count = need,
						owner = self.id,
						pile = self.input,
					}
					projects[p] = true
				end
			end
		end
		if self.need_operator then
			-- need operator
			if operate_finish then
				output_take(self.output, desc.output)
				machine.reset(self.machine)
			elseif not self.operate and input_consume(self.input, desc.input) then
				local p = schedule.new {
					type = "operate",
					x = self.x,
					y = self.y,
					owner = self.id,
					machine = self.machine,
				}
				projects[p] = true
				self.operate = p
			end
		else
			-- auto operate
			local status = machine.status(self.machine)
			if status == "stop" then
				if input_consume(self.input, desc.input) then
					machine.start(self.machine, 1)
				end
			elseif status == "finish" then
				output_take(self.output, desc.output)
				machine.reset(self.machine)
			end
		end
	end
	
	function M:debug()
		debuginfo.add("box", self.id, machine.info(self.machine))
	end
	
	function M:init()
		local x = assert(self.x)
		local y = assert(self.y)
		local building_id = assert(self.building)
		
		local loading
		if self.machine then
			loading = true
		else
			self.machine = machine.add()
		end

		if not self.recipe then
			local desc = datasheet.building[building_id]
			if desc.recipe then
				machine.set_recipe(self.machine, desc.recipe)
			end
			self.need_operator = desc.operator
			self.recipe = desc.recipe
		end
		
		self.input = self.input or container.add_pile()
		self.output = self.output or container.add_pile()
		
		local project = {}
		self.project = project
		if loading then
			local live_projects = schedule.list()
			for id, p in pairs(live_projects) do
				if p.owner == self.id then
					project[id] = true
					if p.type == "operate" then
						self.operate = id
					elseif p.type == "deliver" then
						self.deliver = id
					end
				end
			end
		end
	
		return { what = "box", action = "new", id = self.id, building = self.building, x = x, y = y }
	end
	
	local function pile_to_loot(self, pile)
		local c = container.pile_content(pile)
		for _, type in ipairs(c) do
			container.add_loot(self.x, self.y, type, container.pile_stock(pile, type))
		end
	end
	
	function M:deinit()
		machine.del(self.machine)
		-- drop input and output
		pile_to_loot(self, self.input)
		pile_to_loot(self, self.output)
		container.del_pile(self.input)
		container.del_pile(self.output)
		return { what = "box", action = "del", id = self.id }
	end
	
	function M:export(list)
		local obj = {
			name = self.name,
			id = self.id,
			x = self.x,
			y = self.y,
			building = self.building,
			machine = self.machine,
			recipe = self.recipe,
			input = self.input,
			output = self.output,
		}
		list[#list+1] = obj
	end
	
	return M
end

