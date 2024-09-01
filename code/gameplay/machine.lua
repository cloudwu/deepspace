local C = require "game.machine"
local datasheet = require "gameplay.datasheet"

return function(inst)
	local powergrid = inst.powergrid._data
	local data = C.new()
	local machine = {}

	function machine.add_blueprint(typeid, id)
		local id = id or data:add()
		data:set_recipe(id, datasheet.blueprint[typeid])
		return id
	end
	
	function machine.add()
		return data:add()
	end
	
	function machine.set_recipe(id, typeid)
		data:set_recipe(id, datasheet.recipe[typeid])
	end
	
	function machine.del(id)
		data:remove(id)
	end
	
	function machine.start(id, productivity)
		data:set_productivity(id, productivity)
	end
	
	function machine.reset(id)
		data:set_productivity(id, 0)
		data:reset(id)
	end
	
	function machine.finish(id)
		return data:status(id) == "finish"
	end
	
	function machine.status(id)
		return data:status(id)
	end
	
	function machine.info(id)
		return string.format("%.2g%%", data:info(id, "workprocess") * 100)
	end
	
	function machine.update()
		data:tick(powergrid)
	end

	function machine.export(file)
		local temp = data:export()
		if next(temp) then
			file:write_list("machine", temp)
		end
	end

	function machine.import(savedata)
		local obj = savedata.machine
		if obj then
			data:import(obj)
		end
	end
	
	function machine.clear()
		data:import()
	end
	
	return machine
end