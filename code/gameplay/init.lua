local scene = require "gameplay.scene"
local floor = require "gameplay.floor"
local worker = require "gameplay.worker"
local box = require "gameplay.box"
local actor = require "gameplay.actor"
local blueprint = require "gameplay.blueprint"
local datasheet = require "gameplay.datasheet"
local schedule = require "gameplay.schedule"

local gameplay = {}

local function assign_task(schedule, actor)
	local assign_table = schedule.update()
	if assign_table then
		for task_id, proj in pairs(assign_table) do
			for worker_id, get in pairs(proj.workers) do
				if get then
					actor.message(worker_id, "assign_task", task_id)
				else
					actor.message(worker_id, "reject_task", task_id)
				end
			end
		end
	end
end

local function new_game()
	local message = {}
	
	local function new_message(t)
		message[#message+1] = t
	end
	
	local inst = {}
	local scene = scene()
	local floor = floor(scene)
	local worker = worker(scene)
	local box = box(scene)
	local blueprint = blueprint(scene)
	local schedule = schedule(scene)
	
	inst.scene = scene
	inst.floor = floor
	inst.worker = worker
	inst.box = box
	inst.blueprint = blueprint
	inst.schedule = schedule
	
	local actor = actor(inst)

	function inst.game_start()
		new_message { what = "scene", action = "new", x = scene.x, y = scene.y }
	end
	
	function inst.update()
		actor.update()
		
		floor.update(message)
		scene.update(message)
		worker.update(message)
		box.update(message)
		blueprint.update(message)
		
		assign_task(schedule, actor)
		
		local r = message
		message = {}
		return r
	end
	
	return inst
end

local command = {}
local instance

function command:add_floor(x1, x2, y1, y2)
	self.floor.add(x1, x2, y1, y2)	
end

function command:remove_floor(x1, x2, y1, y2)
	self.floor.remove(x1, x2, y1, y2)	
end

function command:add_worker(x, y)
	self.actor.new {
		name = "worker",
		x = x,
		y = y,
	}
end

function command:add_box(x, y)
	self.box.add(x, y)
end

local building_id <const> = datasheet.building_id.dummy

function command:add_blueprint(x, y)
	self.actor.new {
		name = "building",
		building = building_id,
		x = x,
		y = y,
	}
end

function command.new_game()
	instance = new_game()
	instance.game_start()
end

function gameplay.action(what, ... )
	command[what](instance, ...)
end

function gameplay.update()
	return instance.update()
end

return gameplay
