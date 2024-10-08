local hud = require "hud"
local camera = require "visual.camera"
local vscene = require "visual.scene"
local vfloor = require "visual.floor"
local vworker = require "visual.worker"
local vloot = require "visual.loot"
local vbox = require "visual.box"
local vblueprint = require "visual.blueprint"
local gameplay = require "gameplay"
local datasheet = require "gameplay.datasheet"

local game = {}

local show_debug = false
local show_info = false
function game.keyboard(key)
	if key == "F8" then
		show_debug = not show_debug
		ant.show_debug(show_debug)
	elseif key == "F7" then
		show_info = not show_info
		hud.show("info", show_info)
		if show_info then
			local model = hud.model "info"
			model.title = "Hello"
			model.content = "World"
		end
	elseif key == "F5" then
		print("Saving")
		gameplay.action "save"
	elseif key == "F6" then
		print("Loading")
		gameplay.action "load"
	end
end

function game.mouse(btn, state, x, y)
	camera.mouse_ctrl(btn, state, x, y)
	hud.mouse_message(btn, state, x, y)
end

vscene.init()
hud.init()
gameplay.action "new_game"

local action = {}

function action.floor(msg)
	local action = msg.action
	if action == "change" then
		gameplay.action("publish", "map_change")
	else
		vfloor[action](msg.x, msg.y)
	end
end

function action.worker(msg)
	vworker[msg.action](msg)
end

function action.box(msg)
	vbox[msg.action](msg)
end

function action.blueprint(msg)
	vblueprint[msg.action](msg)
end

function action.loot(msg)
	vloot[msg.action](msg)
end

function game.update()
	camera.key_ctrl()
	for _, mode, action, x1, y1, x2, y2 in hud.message() do
		if mode == "build" then
			if action then
				gameplay.action("add_floor", x1, y1, x2, y2)
				vfloor.draging()
			else
				vfloor.draging("build", x1, y1, x2, y2)
			end
		elseif mode == "remove" then
			if action then
				gameplay.action("remove_floor", x1, y1, x2, y2)
				vfloor.draging()
			else
				vfloor.draging("remove", x1, y1, x2, y2)
			end
		elseif mode == "deck" then
			if action then
				gameplay.action("build_floor", x1, y1, x2, y2)
				vfloor.draging()
			else
				vfloor.draging("build", x1, y1, x2, y2)
			end
		elseif mode == "change_mode" then
			vfloor.draging()
		elseif mode == "worker" then
			if action == "tap" then
				gameplay.action("add_worker", x1, y1)
			end
		elseif mode == "box" then
			if action == "tap" then
				gameplay.action("add_warehouse", x1, y1)
			end
		elseif mode == "iron" or mode == "wood" then
			if action == "tap" then
				local id = assert(datasheet.material_id[mode])
				-- todo: add count ~= 10
				gameplay.action("add_material", id, 10, x1, y1)
			end
		elseif mode == "blueprint" or mode == "furnace" then
			if action == "tap" then
				if mode == "blueprint" then
					gameplay.action("add_blueprint", x1, y1, datasheet.building_id.mining)
				else
					gameplay.action("add_blueprint", x1, y1, datasheet.building_id.furnace)
				end
			end
		end
	end
	local x, y = ant.camera_ctrl.screen_to_world(ant.mouse_state.x, ant.mouse_state.y)
	if x then
		x = (x + 0.5) // 1 | 0
		y = (y + 0.5) // 1 | 0
		vfloor.focus(x, y, hud.mode())
	end
	if show_debug then
--		vfloor.debug()
		gameplay.action("publish", "debug")
	end
	vworker.update()
	vloot.update()
	for _, msg in ipairs(gameplay.update()) do
		local f = action[msg.what]
		if f then
			f(msg)
		end
	end
	hud.update()
end

return game