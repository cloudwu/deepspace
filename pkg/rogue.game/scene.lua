local core = require "game.scene"
local config = require "config"
local util = import_package "rogue.util"

local scene = {}

scene.x = config.scene.x
scene.y = config.scene.y

local layer_name = {
	"floor",
	"region",	-- for pathmap
	"pathmap",
}

local layer = util.index_map(layer_name, -1)

local data = core.new {
	x = scene.x,
	y = scene.y,
	layer = #layer_name,
}

local build_set = {}

function scene.set_floor(x, y, enable)
	local v = enable and 1 or 0
	local ov = data:set(v, x, y, layer.floor)
	if v ~= ov then
		local index = y * scene.x + x
		build_set[index] = not build_set[index]
	end
end

function scene.build_region()
	local queue = { layer = layer.region }
	local n = 1
	for index, v in pairs(build_set) do
		if v then
			local x, y = index % scene.x , index // scene.y
			queue[n] = x
			queue[n+1] = y
			n = n + 2
		end
	end
	data:build(queue)
	build_set = {}
end

function scene.pathmap(x, y)
	data:pathmap {
		block = layer.region,
		target = layer.pathmap,
		x = x,
		y = y,
	}
--	scene.debug_floor "pathmap"
end

function scene.path(t)
	t.layer = layer.pathmap
	return data:path(t)
end

function scene.debug_floor(name)
	local layer_id = layer[name]
	local t = {}
	print(scene.x, scene.y)
	for y = 1, scene.y do
		for x = 1, scene.x do
			t[x] = data:get(x-1, y-1, layer_id)
		end
		print(table.concat(t, " "))
	end
end

return scene