local ecs = ...
local world = ecs.world
local w = world.w

local rhwi      = import_package "ant.hwi"
local iviewport = ecs.require "ant.render|viewport.state"
local iom = ecs.require "ant.objcontroller|obj_motion"
local camera_ctrl = ecs.system "camera_ctrl"
local coord = import_package "rogue.util".coord
local math3d = require "math3d"
local mathpkg   = import_package "ant.math"
local mu, mc = mathpkg.util, mathpkg.constant

local camera = {
	x = 0,
	y = 0,
	z = 0,
	yaw = 0,
	pitch = 45,
	distance = 1,
	min = {},
	max = {},
}

local camera_change_meta = { __index = camera }

local camera_change = setmetatable( {} , camera_change_meta )

local function camera_set(_, what, v)
	local oldv = camera[what]
	if oldv == v then
		camera_change[what] = nil
	else
		camera_change[what] = v
	end
end

local camera_vchange = {}
local vchange = setmetatable({}, { __newindex = camera_vchange })

local camera_accesor = setmetatable({ delta = vchange } , {
	__index = camera_change,
	__newindex = camera_set,
	__pairs = function() return next, camera end,
})

local function delta_change()
	for k,v in pairs(camera_vchange) do
		if rawget(camera_change, k) == nil then
			camera_change[k] = camera[k] + v
		end
		camera_vchange[k] = nil
	end
end

local pitch = { axis = mc.XAXIS }
local yaw = { axis = mc.YAXIS }

local function clamp(what)
	local min = camera.min[what]
	if min and camera[what] < min then
		camera[what] = min
	end
	local max = camera.max[what]
	if max and camera[what] > max then
		camera[what] = max
	end
end

function camera_ctrl:start_frame()
	local main_queue = w:first "main_queue camera_ref:in render_target:in"
	local main_camera <close> =	world:entity(main_queue.camera_ref, "camera:in")
	
	camera.view_rect = main_queue.render_target.view_rect
	camera.vpmat = main_camera.camera.viewprojmat

	if next(camera_vchange) then
		local dx = camera_vchange.x
		local dz = camera_vchange.z
		camera_vchange.x = nil
		camera_vchange.z = nil
		delta_change()
		if dx or dz then
			dx = dx or 0
			dz = dz or 0
			local r = math.rad(-camera_change.yaw)
			local sin_r = math.sin(r)
			local cos_r = math.cos(r)
			local z1 = dz * cos_r + dx * sin_r
			local x1 = dx * cos_r - dz * sin_r
			camera_change.x = camera.x + x1
			camera_change.z = camera.z + z1
		end
	end
	if next(camera_change) == nil then
		return
	end

	for k,v in pairs(camera_change) do
		camera[k] = v
		camera_change[k] = nil
	end
	clamp "x"
	clamp "y"
	clamp "z"
	clamp "distance"
	clamp "yaw"
	clamp "pitch"

	pitch.r = math.rad(camera.pitch)
	yaw.r = math.rad(camera.yaw)
	local r = math3d.mul(math3d.quaternion(yaw), math3d.quaternion(pitch))
	local t = math3d.vector(0,0,0-camera.distance,1)
	t = math3d.transform(r, t, 1)
	t = math3d.add(t, math3d.vector(camera.x, camera.y, camera.z))
	iom.set_srt(main_camera, nil, r, t)
end

-- keyboard and mouse

local kb_mb             = world:sub {"keyboard"}
local key_press = {}
local mouse_mb          = world:sub {"mouse"}
local mouse_lastx, mouse_lasty

local icamera_ctrl = camera_accesor
local GuiValue = {
	rot_speed = 2,
	pan_speed = 0.2,
	distance_speed = 0.5,
}

function camera_accesor.screen_to_world(x, y)
	return coord.screen_to_world(icamera_ctrl.vpmat, icamera_ctrl.view_rect, iviewport.scale_xy(x, y))
end

function camera_ctrl:init_world()
	icamera_ctrl.min.x = -10
	icamera_ctrl.max.x = 10
	icamera_ctrl.min.z = -10
	icamera_ctrl.max.z = 10
	icamera_ctrl.min.pitch = 20
	icamera_ctrl.max.pitch = 80
	icamera_ctrl.distance = 10
end

local show_profile = false

function camera_ctrl:frame_update()
    for _, key, press, status in kb_mb:unpack() do
		key_press[key] = press == 1 or press == 2
		if key == "F8" and key_press[key] == false then
			show_profile = not show_profile
		    rhwi.set_profie(show_profile)
		end
	end
	
	local rot_speed = GuiValue.rot_speed
	
	if key_press.Q then
		icamera_ctrl.delta.yaw = rot_speed
	elseif key_press.E then
		icamera_ctrl.delta.yaw = -rot_speed
	end
	
	local pan_speed = GuiValue.pan_speed
	
	if key_press.W then
		icamera_ctrl.delta.z = pan_speed
	elseif key_press.S then
		icamera_ctrl.delta.z = -pan_speed
	end

	if key_press.A then
		icamera_ctrl.delta.x = -pan_speed
	elseif key_press.D then
		icamera_ctrl.delta.x = pan_speed
	end

	if key_press.Y then
		icamera_ctrl.delta.pitch = -rot_speed
	elseif key_press.H then
		icamera_ctrl.delta.pitch = rot_speed
	end
	
	for _, btn, state, x, y in mouse_mb:unpack() do
		if btn == "WHEEL" then
			icamera_ctrl.delta.distance = - state * GuiValue.distance_speed
		elseif btn == "RIGHT" then
			if state == "DOWN" then
				mouse_lastx, mouse_lasty = x, y
			elseif state == "MOVE" then
				local delta_x = x - mouse_lastx
				local delta_y = y - mouse_lasty
				icamera_ctrl.delta.yaw = delta_x * rot_speed / 4
				icamera_ctrl.delta.pitch = delta_y * rot_speed / 4
				mouse_lastx, mouse_lasty = x, y
			else
				mouse_lastx, mouse_lasty = nil, nil
			end
--		elseif btn == "LEFT" then
--			if state == "DOWN" then
--				mouse_lastx, mouse_lasty = x, y
--			elseif state == "MOVE" then
--				local p0 = camera_accesor.screen_to_world(mouse_lastx, mouse_lasty)
--				local p1 = camera_accesor.screen_to_world(x,y)
--				if p0 and p1 then
--					local delta = math3d.tovalue(math3d.sub(p0, p1))
--					icamera_ctrl.x = icamera_ctrl.x + delta[1]
--					icamera_ctrl.z = icamera_ctrl.z + delta[3]
--				end
--				if p1 then
--					mouse_lastx, mouse_lasty = x, y
--				end
--			else
--				mouse_lastx, mouse_lasty = nil, nil
--			end
		end
    end
	
--	local f = camera_accesor.focus
--	local m = world:get_mouse()
--	local p = camera_accesor.screen_to_world(m.x, m.y)
--	if p then
--		local t = math3d.totable(p)
--		f.x , f.y = t[1], t[3]
--	end
end

return camera_accesor
