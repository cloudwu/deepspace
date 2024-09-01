local datasheet = require "gameplay.datasheet"

local deck_id = datasheet.building_id.deck

return function(inst)
	local build = {}

	function build.construction(id, x, y)
		assert(id == deck_id)
		inst.floor.add(x,y,x,y)
	end

	function build.warehouse(id, x, y)
		inst.actor.new {
			name = "warehouse",
			type = id,
			x = x,
			y = y,
		}
	end

	function build.machine(id, x, y)
		inst.actor.new {
			name = "machine",
			building = id,
			x = x,
			y = y,
		}
	end

	local scene = {}
	local scene_map = require "gameplay.scene_map"
	scene_map(scene, inst)
	
	function scene.add_building(id, x, y)
		local desc = datasheet.building[id] or error ("No building " .. id)
		build[desc.type](id, x, y)
	end
	
	function scene.update(message)
		-- todo
	end
	
	return scene
end
