local scene = require "gameplay.scene"
local floor = require "gameplay.floor"
local container = require "gameplay.container"
local actor = require "gameplay.actor"
local datasheet = require "gameplay.datasheet"
local schedule = require "gameplay.schedule"
local loadsave = require "gameplay.loadsave"
local machine = require "gameplay.machine"
local powergrid = require "gameplay.powergrid"
local loot = require "gameplay.loot"

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
	local loot = loot(inst)
	inst.loot = loot
	local powergrid = powergrid()
	inst.powergrid = powergrid
	local machine = machine(inst)
	inst.machine = machine
	local scene = scene(inst)
	inst.scene = scene
	local floor = floor(scene)
	local schedule = schedule(scene)
	
	inst.floor = floor
	inst.schedule = schedule
	
	local actor = actor(inst)
	inst.actor = actor

	function inst.game_start()
		new_message { what = "scene", action = "new", x = scene.x, y = scene.y }
	end
	
	function inst.update()
		machine.update()
		powergrid.update()
		actor.update(message)
		loot.update(message)
		floor.update(message)
		scene.update(message)
		
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

local deck_id = datasheet.building_id.deck

function command:build_floor(x1, x2, y1, y2)
	local r = self.floor.build(x1, x2, y1, y2)
	for i = 1, #r, 2 do
		self.actor.new {
			name = "building",
			building = deck_id,
			x = r[i],
			y = r[i+1],
			near = true,
		}
	end
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

function command:add_warehouse(x, y)
	self.actor.new {
		name = "building",
		building = datasheet.building_id.storage,
		x = x,
		y = y,
	}
end

function command:add_material(id, count, x, y)
	self.container.add_loot(x, y, id, count)
end

function command:add_blueprint(x, y, building)
	self.actor.new {
		name = "building",
		building = building,
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

function gameplay.action(what, ... )
	command[what](instance, ...)
end

local savelist <const> = {
	"container",
	"powergrid",
	"machine",
	"floor",
	"schedule",
	"loot",
}

local function call_savelist(func, ...)
	for _, what in ipairs(savelist) do
		func(what, ...)
	end
end

local clear_message = {
	{ what = "worker", action = "clear" },
	{ what = "blueprint", action = "clear" },
	{ what = "box", action = "clear" },
	{ what = "floor", action = "clear" },
	{ what = "loot", action = "clear" },
}

local function clear_(what, self)
	local f = self[what].clear
	if f then
		f()
	end
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
