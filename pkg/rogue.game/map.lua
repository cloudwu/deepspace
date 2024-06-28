local monitor = require "monitor"
local floor = require "floor"

local map = {}

local data = {}
local wall = {}

local LINE <const> = 0x1000000

local function slot(x, y, set)
	local idx = x << 24 | y
	local c = data[idx] or 0
	if set == ((c & 1) ~= 0) then
		-- not change
		return
	end
	local n_index = idx - 1
	local s_index = idx + 1
	local w_index = idx - LINE
	local e_index = idx + LINE
	local n = data[n_index] or 0
	local s = data[s_index] or 0
	local w = data[w_index] or 0
	local e = data[e_index] or 0
	local slot = set and 1 or 0
	local edge_n = slot ~ (n & 1)
	local edge_w = slot ~ (w & 1)
	local south_edge_n = slot ~ (s & 1)
	local east_edge_w = slot ~ (e & 1)

	-- Cornor (W   S   E  N) N-Edge W-Edge Slot
	--        64  32  16  8   4      2     1
	local cornor = ((n & 2) >> 1 ) | (edge_n << 1) | (edge_w << 2) | ((w & 4) << 1)
	wall[idx] = cornor
	data[idx] = cornor << 3 | edge_n << 2 | edge_w << 1 | slot
	e = (e & ~(2 | 32 | 64)) | (east_edge_w * (2 | 32)) | (edge_n << 6) -- fix east slot (edge_w and cornor)
	data[e_index] =	e
	wall[e_index] = e >> 3
	s = (s & ~(4 | 8 | 16)) | (south_edge_n * (4 | 16)) | (edge_w << 3) -- fix south slot (edge_n and cornor)
	data[s_index] =	s
	wall[s_index] =	s >> 3
	local se_index = e_index + 1
	local se = data[se_index] or 0
	se = (se & ~(8 | 64)) | (east_edge_w << 3) | (south_edge_n << 6) -- fix south-east cornor
	data[se_index] = se
	wall[se_index] = se >> 3
end

local function slot_range(set, x1, x2, y1, y2)
	x1, x2, y1, y2 = floor.drag_clamp(x1, x2, y1, y2)
	for x = x1, x2 do
		for y = y1, y2 do
			slot(x, y, set)
		end
	end
end

function map.add(...)
	slot_range(true, ...)
end

function map.del(...)
	slot_range(false, ...)
end

--      1 N
-- 8 W  +    2 E
--      4 S

local wall_type = {
	{ "i",	0   },  -- 1
	{ "i",	90  },  -- 2
	{ "l",  180 },  -- 3	NE
	{ "i",	180 },  -- 4
	{ "i",	90  },  -- 5
	{ "l",	270 },  -- 6	ES
	{ "t",	90  },  -- 7	NES
	{ "i",	270 },  -- 8
	{ "l",	90  },  -- 9	NW
	{ "i",	0   },  -- 10
	{ "t",	0   },  -- 11	NEW
	{ "l",	0   },  -- 12	WS
	{ "t",	270 },  -- 13	WNS
	{ "t",	180 },  -- 14	WES
	{ "x",	0   },  -- 15
}

local wall_obj = {}

local wall_prefab = {}
for _, t in ipairs { "i", "l", "t", "x" } do
	wall_prefab[t] = "/asset/wall/".. t ..".glb/mesh.prefab"
end

local function new_wall(world, t, x, y, r)
	local proxy = { x = x, y = y, r = r, material = { visible = true } }
	world:create_instance {
		prefab = wall_prefab[t],
		on_ready = function (inst)
			local eid_list = inst.tag["*"]
			local eid = eid_list[1]
			proxy.eid = eid
			local mat = proxy.material
			proxy.material = monitor.material(world, eid_list)
			proxy.material.visible = mat.visible
			monitor.new(proxy)
		end,
	}
	return proxy
end

function map.flush(world)
	local n = 1
	for idx, w in pairs(wall) do
		local x = idx >> 24
		local y = idx & 0xffffff
		local obj = wall_obj[idx]
		if not obj then
			if w ~= 0 then
				local wall = wall_type[w]
				local t = wall[1]
				obj = { [t] = new_wall(world, t, x-0.5, y-0.5, wall[2]) }
				wall_obj[idx] = obj
			end
		else
			for _, v in pairs(obj) do
				v.material.visible = false
			end
			if w ~= 0 then
				local wall = wall_type[w]
				local t = wall[1]
				local proxy = obj[t]
				if proxy then
					proxy.x = x - 0.5
					proxy.y = y - 0.5
					proxy.r = wall[2]
					proxy.material.visible = true
				else
					obj[t] = new_wall(world, t, x-0.5, y-0.5, wall[2])
				end
			end
		end
		wall[idx] = nil
	end
end

return map
