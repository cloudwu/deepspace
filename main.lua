local ltask = require "ltask"
ltask.spawn ("rogue.mod|loader", (...))

import_package "ant.window".start {
	window_size = "1280x720",
	vsync = true,
--	profile = true,
	log = "info",
	feature = {
		"rogue.game",
		"ant.render",
		"ant.animation",
		"ant.shadow_bounding|scene_bounding",
		"ant.pipeline",
		"ant.sky|sky",
	},
}
