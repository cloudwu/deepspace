local ltask = require "ltask"
ltask.spawn ("rogue.mod|loader", (...))
import_package "ant.framework.demo".start {
	window_size = "1280x720",
	vsync = true,
--	profile = true,
	log = "info",
	path = "/code/main.lua",
--	path = "/uitest/hud.lua",
	setting = "/code/config.ant",
}
