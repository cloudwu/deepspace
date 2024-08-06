return function()
	local scene = {}
	local scene_map = require "gameplay.scene_map"
	scene_map(scene)
	
	function scene.update(message)
		-- todo
	end
	
	return scene
end
