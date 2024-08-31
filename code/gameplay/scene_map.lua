local core = require "game.scene"
local pathmap = require "game.pathmap"
local util = require "util"
local setting = ant.setting

local layer_name = {
	"floor",
	"region",	-- for pathmap
}
local layer = util.index_map(layer_name, -1)

return function (scene, inst)
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
--		print(data:debug(layer.region))
		build_set = {}
		map_dirty = false
	end

	local function rebuild()
		build_region()
		path:reset()
	end

	local path_func = {
--		[true] = function (self, p, x, y)
--			local r = path.path_near(self, p, x, y)
--			print(path.debug(self, x, y, true))
--			return r
--		end,
		[true] = path.path_near,
		[false] = path.path,
--		[false] = function (self, p, x, y)
--			local r = path.path(self, p, x, y)
--			print(path.debug(self, x, y, false))
--			return r
--		end,
	}

	local dist_func = {
		[true] = path.dist_near,
		[false] = path.dist,
	}

	function scene.path(t, x, y, near)
		if map_dirty then
			rebuild()
		end
		return path_func[near == true](path, t, x, y)
	end
	
	function scene.dist(x1, y1, x2, y2, near)
		if map_dirty then
			rebuild()
		end
		local f = dist_func[near == true]
		return f(path, x1, y1, x2, y2)
	end
	
	local max_dist = math.huge

	function scene.nearest(x, y, storage_list)
		if not storage_list then
			return nil, max_dist
		end
		if map_dirty then
			rebuild()
		end
		local min_dist = max_dist, min_index
		for i = 1, #storage_list do
			local v = storage_list[i]
			local id = v >> 32
			local sx = (v >> 16) & 0xffff
			local sy = v & 0xffff
			local dist = path:dist(sx, sy, x, y)
			if dist > 0 then
				if dist < min_dist then
					min_dist = dist
					min_index = i
				end
			end
		end
		if min_index then
			return min_index, min_dist
		end
	end

	function scene.reachable(x, y, storage_list)
		if map_dirty then
			rebuild()
		end
		for i = 1, #storage_list do
			local v = storage_list[i]
			local sx = (v >> 16) & 0xffff
			local sy = v & 0xffff
			local dist = path:dist(sx, sy, x, y)
			if dist > 0 then
				return true
			end
		end
	end

	function scene.export_floor()
		return data:export(layer.floor)
	end

	function scene.clear_floor()
		data:clear(layer.floor)
		data:clear(layer.region)
		map_dirty = true
		build_set = {}
	end
end
