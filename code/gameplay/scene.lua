local datasheet = require "gameplay.datasheet"

local deck_id = datasheet.building_id.deck

return function(inst)
	local scene = {}
	local scene_map = require "gameplay.scene_map"
	scene_map(scene, inst)
	
	function scene.add_building(building_id, x, y)
		-- todo:
--		print("BUILD", building, x, y)
		if building_id == deck_id then
			inst.floor.add(x,y,x,y)
		end
	end
	
	function scene.update(message)
		-- todo
	end
	
	return scene
end
