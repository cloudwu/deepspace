local C = require "game.machine"
local datasheet = require "gameplay.datasheet"

return function(inst)
	local powergrid = inst.powergrid._data
	local data = C.new()
	local obj = {}
	local machine = {}

	function machine.add_blueprint(typeid, id)
		local id = id or data:add()
		obj[id] = { type = "blueprint", id = typeid }
		data:set_recipe(id, datasheet.blueprint[typeid])
		return id
	end
	
	function machine.start_blueprint(id, productivity)
		data:set_productivity(id, productivity)
	end
	
	function machine.finish_blueprint(id)
		return data:status(id) == "finish"
	end
	
	function machine.status(id)
		return data:info(id)
	end
	
	function machine.info(id)
		return string.format("Building: %.2g%%", data:info(id, "workprocess") * 100)
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
		obj = {}
	end
	
	return machine
end