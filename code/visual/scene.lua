local scene = {}

function scene.init()
	ant.maxfps(ant.setting.fps)
	ant.sprite2d_base(ant.setting.sprite_base)
	-- set camera
	
	ant.camera_ctrl.x = ant.setting.scene.x / 2
	ant.camera_ctrl.y = ant.setting.scene.y / 2

	ant.camera_ctrl.min.x = 0
	ant.camera_ctrl.max.x = ant.setting.scene.x
	ant.camera_ctrl.min.y = 0
	ant.camera_ctrl.max.y = ant.setting.scene.y

	-- skybox
	ant.import_prefab "/asset/light.prefab"
end

return scene