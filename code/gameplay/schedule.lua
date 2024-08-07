local allocid = require "gameplay.allocid"

return function (scene)
	local schedule = {}
	
	local alloc_id = allocid()

	local project = {}

	function schedule.new(args)
		local id = alloc_id(args.id)
		args.id = id
		project[id] = args
		return id
	end
	
	function schedule.list()
		return project
	end
	
	local assign_table = {}
	local cancel_table = {}
	
	function schedule.request(id, worker_id, priority)
		local a = assign_table[id]
		if not a then
			a = { priority = 0, workers = {} }
			assign_table[id] = a
		end
		local workers = a.workers
		local p = project[id]
		if p and p.worker == nil then
			if priority > a.priority then
				if a.candidate then
					workers[a.candidate] = false
				end
				a.candidate = worker_id
				a.priority = priority
				workers[worker_id] = true
			else
				workers[worker_id] = false
			end
		else
			-- project not exist
			workers[worker_id] = false
		end
	end
	
	function schedule.cancel(id, worker_id)
		local p = project[id]
		if p then
			assert(p.worker == worker_id)
			cancel_table[id] = true
		end
	end
	
	function schedule.complete(id, worker_id)
		local p = project[id]
		assert(p and p.worker == worker_id, ("p.worker = %d %d"):format(p.worker, worker_id))
		project[id] = nil
	end
	
	function schedule.update()
		for id in pairs(cancel_table) do
			project[id].worker = nil
			cancel_table[id] = nil
		end
		if not next(assign_table) then
			return
		end
		local r = assign_table
		for id, proj in pairs(assign_table) do
			project[id].worker = proj.candidate
		end
		assign_table = {}
		return r		
	end
	
	function schedule.export(file)
		if next(project) then
			local list = {}
			local n = 1
			for id, p in pairs(project) do
				list[n] = p
				n = n + 1
			end
			file:write_list("project", list)
		end
	end
	
	function schedule.clear()
		project = {}
		assign_table = {}
		cancel_table = {}
		alloc_id = allocid()
	end
	
	function schedule.import(savedata)
		local obj = savedata.project
		if obj then
			for _, p in ipairs(obj) do
				schedule.new(p)
			end
		end
	end

	return schedule
end