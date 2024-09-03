local ltask = require "ltask"
local game = {}

local window = ant.gui_open "/ui/test.html"

function game.keyboard(key)
	if key == "F5" then
		print "Reload"
		window.close()
		ltask.spawn "rogue.mod|loader"
		window = ant.gui_open "/ui/test.html"
	end
end

function game.update()
end

return game
