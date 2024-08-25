local scene = require "gameplay.scene"
local floor = require "gameplay.floor"
local worker = require "gameplay.worker"
local container = require "gameplay.container"
local box = require "gameplay.box"
local actor = require "gameplay.actor"
local blueprint = require "gameplay.blueprint"
local datasheet = require "gameplay.datasheet"
local schedule = require "gameplay.schedule"
local loadsave = require "gameplay.loadsave"
local machine = require "gameplay.machine"
local powergrid = require "gameplay.powergrid"

local savefile <const> = "savetest.ant"

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
	local container = container()
	inst.container = container
	local powergrid = powergrid()
	inst.powergrid = powergrid
	local machine = machine(inst)
	inst.machine = machine
	local scene = scene(inst)
	inst.scene = scene
	local floor = floor(scene)
	local worker = worker(scene)
	local box = box(inst)
	local blueprint = blueprint(scene)
	local schedule = schedule(scene)
	
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
		machine.update()
		powergrid.update()
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
	local id = self.box.add(x, y)
	local wood = datasheet.material_id.wood
	local iron = datasheet.material_id.iron

	self.box.put(id, wood, 200)
	self.box.put(id, iron, 100)
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

local function is_prefix(s, prefix)
	local n = #prefix
	return s:sub(1, n) == prefix
end

function command:save()
	self.request_save = true
end

function command:load()
	self.request_load = true
end

function command.new_game()
	instance = new_game()
	instance.game_start()
end

function command:publish(msg)
	instance.actor.publish(msg)
end

function command:show_debug()
	self.box.debug()
end

function gameplay.action(what, ... )
	command[what](instance, ...)
end

local savelist <const> = {
	"container",
	"powergrid",
	"machine",
	"floor",
	"box",
	"schedule",
	"worker",
	"blueprint",
}

local function call_savelist(func, ...)
	for _, what in ipairs(savelist) do
		func(what, ...)
	end
end

local clear_message = {}

local function gen_clear_message(what)
	clear_message[#clear_message+1] = { what = what, action = "clear" }
end

call_savelist(gen_clear_message)

local function clear_(what, self)
	self[what].clear()
end

local function import_(what, self, data)
	local f = self[what].import
	if f then
		f(data)
	end
end

local function update_(what, self, messages)
	local f = self[what].update
	if f then
		f(messages)
	end
end

function gameplay.update()
	local r = instance.update()
	if instance.request_save then
		instance.request_save = false
		local f <close> = loadsave.savefile(savefile)
		for _, what in ipairs(savelist) do
			local export_func = instance[what].export
			if export_func then
				export_func(f)
			end
		end
		instance.actor.export(f)
	end
	if instance.request_load then
		instance.request_load = false
		local data = loadsave.load(savefile)
		if data then
			call_savelist(clear_, instance)
			call_savelist(import_, instance, data)
			local reset_message = table.move(clear_message, 1, #clear_message, 1, {})
			instance.actor.import(data)
			call_savelist(update_, instance, reset_message)
			return reset_message
		else
			print("No file : ", savefile)
		end
	end
	return r
end

return gameplay
