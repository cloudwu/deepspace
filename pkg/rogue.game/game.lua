local ecs = ...
local world = ecs.world

local scene = require "scene"
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
local mouse_events = ecs.require "mouse_events"
local monitor = require "monitor"
local savefile = "save.ant"
local config = require "config"

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

local avatar = { x = 0, y = 0, r = 0 , material = { visible = false } }

local function init_avatar()
	world:create_entity {
		policy = {
			"ant.render|render",
		},
		data = {
			scene 		= {	s = 1 },
			visible_masks = "main_view|cast_shadow",
			material	= "/asset/avatar.material",
			visible	    = true,
            render_layer = "translucent",
			feature_set = {
				SHADOW_ALPHA_MASK = true,
			},
			mesh		= "quad.primitive",
			on_ready = function (e)
				world.w:extend(e, "eid:in")
				avatar.eid = e.eid
				local mat = avatar.material
				avatar.material = monitor.material(world, { e.eid })
				avatar.material.visible = mat.visible
				monitor.new(avatar)
			end
		}
	}
end

function game:init_world()
	bgfx.maxfps(config.fps)
	map.load(world, savefile)
	scene.build_region()
	world:create_instance {	prefab = "/asset/light.prefab" }
	monitor.set_coord(floor.width/2-0.5, floor.height/2-0.5)
	init_avatar()
end

local avatar_path = {}

local function set_avatar(x, y)
	avatar.x = x
	avatar.y = y
	avatar_path.x, avatar_path.y = nil
	avatar.material.visible = true
	scene.pathmap(x, y)
--	scene.debug_floor "region"
--	scene.debug_floor "pathmap"
end

local function update_speed()
	local n = #avatar_path
	if n < 4 then
		avatar_path.dx = nil
		avatar_path.dy = nil
		scene.pathmap(avatar.x // 1, avatar.y // 1)
		return
	end
	local tx, ty = avatar_path[n - 3], avatar_path[n - 2]
	local fx, fy = avatar_path[n - 1], avatar_path[n]
	
	local d = ((fx - tx) ^ 2 + (fy - ty) ^ 2) ^ 0.5
	local step = (d / config.speed * config.fps + 0.5) // 1
	local inv = 1 / step
	avatar_path.dx = (tx - fx) * inv
	avatar_path.dy = (ty - fy) * inv
	avatar_path.step = step
end

local function update_path()
	if avatar_path.x == nil then
		return
	end
	local dist = scene.path(avatar_path)
	if dist > 0 and avatar_path[1] then
--		print_r(avatar_path)
		local n = #avatar_path
		avatar_path[n-1] = avatar.x
		avatar_path[n] = avatar.y
		update_speed()
	end
end

local function move_avatar(x,y)
	local fx = math.floor(avatar.x + 0.5)
	local fy = math.floor(avatar.y + 0.5)
	scene.pathmap(fx,fy)
	avatar_path.x, avatar_path.y = x, y
	update_path()
end

local function update_avatar()
	if avatar_path.dx then
		avatar.x = avatar.x + avatar_path.dx
		avatar.y = avatar.y + avatar_path.dy
		local step = avatar_path.step - 1
		if step == 0 then
			local n = #avatar_path
			avatar_path[n] = nil
			avatar_path[n-1] = nil
			avatar.x = avatar_path[n-3]
			avatar.y = avatar_path[n-2]
			update_speed()
		else
			avatar_path.step = step
		end
	end
	avatar.r = camera.yaw
end

local key_mb	= world:sub { "keyboard" }
local cancel_drag

local drag_mode = {
	mode = "add",
	add = {
		next = "del",
		color = 0x000020,
		action = function (world, x1, x2, y1, y2)
			floor.add(world, x1, x2, y1, y2)
			map.add(x1, x2, y1, y2)
		end,
		draging = function (world, x1, x2, y1, y2)
			floor.drag(world, x1, x2, y1, y2, 0x000020)
			floor.focus_clear()
		end,
	},
	del = {
		next = "move",
		color = 0x200000,
		action = function (world, x1, x2, y1, y2)
			floor.del(world, x1, x2, y1, y2)
			map.del(x1, x2, y1, y2)
		end,
		draging = function (world, x1, x2, y1, y2)
			floor.drag(world, x1, x2, y1, y2, 0x200000)
			floor.focus_clear()
		end,
	},
	move = {
		next = "add",
		color = 0x202000,
		action = function (world, x1, x2, y1, y2)
			local x, y = x2, y2
			if floor.exist(x,y) then
				if avatar.material.visible then
					move_avatar(x,y)
				else
					set_avatar(x,y)
				end
			else
				avatar.material.visible = false
			end
		end,
	},
}

local function key_events()
	for _, key, press in key_mb:unpack() do
		press = press == 1
		if key == "P" and not press then
--			scene.build_region()
--			scene.debug_floor()
		end
	end
end

function game:data_changed()
	key_events()
	for _, event, x1, x2, y1, y2 in mouse_events.mouse_events() do
		if event == "drag_end" then
			local drag = drag_mode[drag_mode.mode]
			floor.drag_clear(world)
			if not cancel_drag then
				drag.action(world, x1, x2, y1, y2)
			end
		elseif event == "drag_begin" then
			cancel_drag = false
		elseif event == "draging" then
			local drag = drag_mode[drag_mode.mode]
			floor.drag_clear(world)
			if not cancel_drag then
				if drag.draging then
					drag.draging(world, x1, x2, y1, y2)
				end
			end
		elseif event == "rightclick" then
			local moved, draging = x1, x2
			if draging then
				cancel_drag = true
				floor.drag_clear(world)
			elseif not moved then
				drag_mode.mode = drag_mode[drag_mode.mode].next
			end
		elseif event == "hover" then
			local x, y = x1, x2
			local color = drag_mode[drag_mode.mode].color
			floor.focus(world, x, y, color)
		end
	end
	if map.flush(world) then
		-- world changes
		scene.build_region()
		if avatar.material.visible then
			local ax = math.floor(avatar.x + 0.5)
			local ay = math.floor(avatar.y + 0.5)
			if not floor.exist(ax,ay) then
				avatar.material.visible = false
			else
				scene.pathmap(ax, ay)
				update_path()
			end
		end
		
		map.save(savefile)
	end
	update_avatar()
	monitor.flush(world)
end