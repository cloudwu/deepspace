local core = require "game.scene"
local util = require "util"
local setting = ant.setting

local layer_name = {
	"floor",
	"region",	-- for pathmap
	"pathmap_storage",
}
local layer = util.index_map(layer_name, -1)
local pathmap_n = setting.pathmap
local name_layer_n = #layer_name

return function (scene)
	scene.x = setting.scene.x
	scene.y = setting.scene.y
	local data = core.new {
		x = scene.x,
		y = scene.y,
		layer = pathmap_n + name_layer_n,
	}

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
	
--	local function debug_map(layer)
--		local temp = {}
--		for y = 0, scene.y -1 do
--			for x = 0, scene.x - 1 do
--				local v = data:get(x, y, layer)
--				temp[x+1] = v
--			end
--			print(table.concat(temp, " "))
--		end
--	end

	local function build_storage_pathmap()
		local t = {
			block = layer.region,
			target = layer.pathmap_storage,
		}
		local n = 1
		for index in pairs(storage) do
			local x, y = index >> 16 , index & 0xffff
			t[n] = x
			t[n+1] = y
			n = n + 2
		end
		data:pathmap(t)
	end

	local pathmap_cache = {} -- index -> pos

	local function rebuild()
		build_region()
		build_storage_pathmap()
		pathmap_cache = {}
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
		t.layer = layer.pathmap_storage
		return data:path(t)
	end
	
	function scene.storage_dist(x, y)
		gen_storage_pathmap()
		return data:get(x, y, layer.pathmap_storage)
	end
	
	local temp = { block = layer.region }
	
	local function gen_pathmap(x, y)
		if map_dirty then
			rebuild()
		end
		local pos = x << 16 | y
		local index = pos % pathmap_n
		local ret = index + name_layer_n
		if pathmap_cache[index] ~= pos then
			temp.target = ret
			temp[1] = x
			temp[2] = y
			data:pathmap(temp)
			pathmap_cache[index] = pos
		end
		return ret
	end
	
	function scene.path(t, x, y)
		t.layer = gen_pathmap(x,y)
		return data:path(t)
	end
	
	function scene.dist(x1, y1, x2, y2)
		return data:get(x2, y2, gen_pathmap(x1,y1))
	end
end
