local ecs = ...
local world = ecs.world

local util = import_package "rogue.util"
local print_r = util.print_r
local async_instance = util.async(world)

local game	= ecs.system "game"

local iani = ecs.require "ant.anim_ctrl|state_machine"
local iloader = ecs.require "ant.anim_ctrl|loader"
local bgfx = require "bgfx"
local map = require "map"
local floor = require "floor"
local camera = ecs.require "camera_ctrl"
local math3d = require "math3d"
local monitor = require "monitor"
local savefile = "save.ant"

--local core = require "game.core"
--print(core.test())

local function object2d(what, x, y, r)
	local proxy = { x = x, y = y, r = r or 0 }
	world:create_instance {
		prefab = "/asset/wall/"..what .. ".glb/mesh.prefab",
		visible = false,
		on_ready = function (inst)
			local eid_list = inst.tag["*"]
			local eid = eid_list[1]
			proxy.eid = eid
			proxy.material = monitor.material(world, eid_list)
			if what == "floor" then
				proxy.material.emissive = 0x000040
			end
			monitor.new(proxy)
		end,
	}
	return proxy
end

--[[
local function hero()
	async_instance(function(async)
--		local ani = async:create_instance { prefab = "/asset/avatar.ani.glb/animation.prefab" }
		local inst = async:create_instance { prefab = "/asset/avatar.glb/mesh.prefab" }
		local ani = inst.tag.animation[1]
		iloader.load(ani, "/asset/avatar.ani.glb")
		iani.play(ani, { name = "walk", loop = true })
	end)
end
]]

local avatar = { x = floor.width/2, y = floor.height/2, r = 0 }

local function init_avatar()
	world:create_entity {
		policy = {
			"ant.render|render",
		},
		data = {
			scene 		= {	s = 1 },
--			visible_masks = "main_view|cast_shadow",
			material	= "/asset/avatar.material",
			visible	    = true,
            render_layer = "translucent",
			mesh		= "quad.primitive",
			on_ready = function (e)
				world.w:extend(e, "eid:in")
				avatar.eid = e.eid
				monitor.new(avatar)
			end
		}
	}
end

function game:init_world()
	bgfx.maxfps(50)
	map.load(world, savefile)
	world:create_instance {	prefab = "/asset/light.prefab" }
	monitor.set_coord(floor.width/2-0.5, floor.height/2-0.5)
	init_avatar()
end

local mouse_mb          = world:sub {"mouse"}
local start_x, start_y, cur_x, cur_y, cancel_drag

local function map_coord(x, y)
	local p = camera.screen_to_world(x, y)
	if p then
		local pos = math3d.tovalue(p)
		x, y = monitor.get_coord(pos[1], pos[3])
		x = (x + 0.5) // 1 | 0
		y = (y + 0.5) // 1 | 0
		return x, y
	end
end

local drag_mode = {
	mode = "add",
	add = {
		next = "del",
		color = 0x000020,
		action = function (world, x1, x2, y1, y2)
			floor.add(world, x1, x2, y1, y2)
			map.add(x1, x2, y1, y2)
		end
	},
	del = {
		next = "add",
		color = 0x200000,
		action = function (world, x1, x2, y1, y2)
			floor.del(world, x1, x2, y1, y2)
			map.del(x1, x2, y1, y2)
		end
	},
}

local lastx, lasty, moved

function game:data_changed()
	for _, btn, state, x, y in mouse_mb:unpack() do
		if btn == "LEFT" then
			x, y = map_coord(x, y)
			if state == "DOWN" then
				start_x = x or start_x
				start_y = y or start_y
				cur_x, cur_y = start_x, start_y
			elseif state == "MOVE" then
				cur_x = x or cur_x
				cur_y = y or cur_y
				if not start_x then
					start_x, start_y = cur_x, cur_y
				end
			else
				-- state == "UP"
				local drag = drag_mode[drag_mode.mode]
				floor.drag_clear(world)
				if not cancel_drag then
					drag.action(world, start_x, cur_x, start_y, cur_y)
				else
					cancel_drag = false
				end
				start_x = nil
				start_y = nil
				cur_x = nil
				cur_y = nil
			end
		elseif btn == "RIGHT" then
			if state == "MOVE" then
				moved = true
			elseif state == "UP" then
				if start_x then
					-- drag
					cancel_drag = not cancel_drag
				elseif not moved then
					drag_mode.mode = drag_mode[drag_mode.mode].next
				end
				moved = false
			end
		end
	end

	local mouse = world:get_mouse()
	local x, y = map_coord(mouse.x, mouse.y)
	if x ~= lastx or y ~=lasty then
		lastx, lasty = x, y
	end
	
	local color = drag_mode[drag_mode.mode].color
	
	if start_x then
		floor.drag_clear(world)
		if not cancel_drag then
			floor.drag(world, start_x, cur_x, start_y, cur_y, color)
		end
		floor.focus_clear()
	else
		floor.focus(world, x, y, color)
	end
	
	if map.flush(world) then
		map.save(savefile)
	end
	avatar.r = camera.yaw
	monitor.flush(world)
end