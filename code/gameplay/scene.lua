return function(inst)
	local scene = {}
	local scene_map = require "gameplay.scene_map"
	scene_map(scene, inst)
	
	function scene.update(message)
		-- todo
	end
	
	return scene
end
