local ltask = require "ltask"

import_package "ant.framework.demo".start {
	window_size = "1280x720",
	vsync = true,
--	profile = true,
	log = "info",
	path = "/uitest/hud.lua",
	setting = "/code/config.ant",
}
