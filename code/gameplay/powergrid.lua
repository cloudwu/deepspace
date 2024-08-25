local C = require "game.powergrid"

return function()
	local data = C.new()
	local powergrid = {}
	
	powergrid._data = data	-- for machine
	
	function powergrid.update()
		data:tick()
	end

	function powergrid.export(file)
		local temp = data:export()
		if next(temp) then
			file:write_list("powergrid", temp)
		end
	end

	function powergrid.import(savedata)
		local obj = savedata.powergrid
		if obj then
			data:import(obj)
		end
	end
	
	function powergrid.clear()
		data:import()
	end
	
	return powergrid
end