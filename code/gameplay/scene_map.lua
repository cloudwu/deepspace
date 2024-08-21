local core = require "game.scene"
local pathmap = require "game.pathmap"
local util = require "util"
local setting = ant.setting

pathmap.dist = pathmap.dist_near
pathmap.path = pathmap.path_near

local layer_name = {
	"floor",
	"region",	-- for pathmap
}
local layer = util.index_map(layer_name, -1)

return function (scene)
	scene.x = setting.scene.x
	scene.y = setting.scene.y
	local data = core.new {
		x = scene.x,
		y = scene.y,
		layer = #layer_name,
	}
	local path = pathmap.new(data, layer.region)

	local map_dirty = false
	local build_set = {}
	function scene.set_floor(x, y, enable)
		local v = enable and 1 or 0
		local ov = data:set(v, x, y, layer.floor)
		if v ~= ov then
			local index = x << 16 | y
			build_set[index] = not build_set[index]
			map_dirty = true
			return true
		end
	end
	
	function scene.valid(x, y)
		local v = data:get(x, y, layer.floor)
		return v == 1
	end
	
	local function build_region()
		local queue = { layer = layer.region }
		local n = 1
		for index, v in pairs(build_set) do
			if v then
				local x, y = index >> 16 , index & 0xffff
				queue[n] = x
				queue[n+1] = y
				n = n + 2
			end
		end
		data:build(queue)
		build_set = {}
		map_dirty = false
	end

	local storage_dirty = false
	local storage = {}
	
	function scene.storage(x, y, enable)
		local index = x << 16 | y
		storage[index] = enable and true or nil
		storage_dirty = true
	end

	local function build_storage_pathmap()
		local t = {}
		local n = 1
		for index in pairs(storage) do
			local x, y = index >> 16 , index & 0xffff
			t[n] = x
			t[n+1] = y
			n = n + 2
		end
		path:storage_map(t)
	end

	local function rebuild()
		build_region()
		build_storage_pathmap()
		path:reset()
	end
	
	local function gen_storage_pathmap()
		if map_dirty then
			rebuild()
		elseif storage_dirty then
			build_storage_pathmap()
		end
		storage_dirty = false
	end
	
	function scene.storage_path(t)
		gen_storage_pathmap()
		return path:storage_path(t)
	end
	
	function scene.storage_dist(x, y)
		gen_storage_pathmap()
		return path:storage_dist(x, y)
	end
	
	local temp = { block = layer.region }
	
	function scene.path(t, x, y)
		if map_dirty then
			rebuild()
		end
		return path:path(t, x, y)
	end
	
	function scene.dist(x1, y1, x2, y2)
		if map_dirty then
			rebuild()
		end
		return path:dist(x1, y1, x2, y2)
	end

	function scene.export_floor()
		return data:export(layer.floor)
	end
	
	function scene.clear_floor()
		data:clear(layer.floor)
	end
end
