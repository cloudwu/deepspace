local seri = import_package "ant.serialize"

local map = {}

local wall_type = {
	[0] = "i",
	"i",	-- 1
	"i",	-- 2
	"t",	-- 3
	"x",	-- 4
}

local wall_subtype = {
	[3] = 'l',	-- e s
	[6] = 'l',	-- s w
	[12] = 'l',	-- w n
	[9] = 'l',	-- n e
}

local wall_face = {
	[0] = 0,
	0,  -- 1: e
	90, -- 2: s
	270,-- 3: e s
	180, -- 4: w
	0,   -- 5: w e
	0,   -- 6: s w
	180,-- 7: e s w
	270, -- 8: n
	180,-- 9: n e
	90, -- 10: n s
	90,-- 11: n e s
	90, -- 12: w n
	0,-- 13: w n e
	270,-- 14: s w n
	0, -- 15: e s w n
}

function map.load(filename)
	local data = seri.load(filename)
	local wall = {
		x = #data[1],
		y = #data,
	}
	local function get(x,y)
		if x < 1 or x > wall.x or y < 1 or y > wall.y then
			return false
		end
		return data[y]:sub(x,x)
	end

	local function internal(x,y)
		local w = get(x,y)
		return w and w ~= "-"
	end
	
	local function gen_external(i,j, ewall)
		ewall.external = true
		local e = get(i+1,j) == "E"
		local s = get(i,j+1) == "E"
		local w = get(i-1,j) == "E"
		local n = get(i,j-1) == "E"
		if e and w then
			if internal(i, j+1) then
				ewall.face = 180
			else
				ewall.face = 0
			end
		elseif s and n then
			if internal(i+1, j) then
				ewall.face = 90
			else
				ewall.face = 270
			end
		elseif n and e then
			ewall.face = 45
		elseif e and s then
			ewall.face = 135
		elseif s and w then
			ewall.face = 225
		elseif w and n then
			ewall.face = 315
		end
	end
	
	local function get_i(x, y)
		local t = get(x,y)
		if t == "E" or t == "i" or t == "=" then
			return 1
		else
			return 0
		end
	end
	
	local function gen_wall(i,j, iwall)
		local e = get_i(i+1,j)
		local s = get_i(i,j+1)
		local w = get_i(i-1,j)
		local n = get_i(i,j-1)
		
		local adj = e + s + w + n
		local t = e + s * 2 + w * 4 + n * 8
		iwall.type = wall_subtype[t] or wall_type[adj]
		iwall.face = wall_face[t]
	end
	
	local wall_n = 1
	
	for i = 1, wall.x do
		for j = 1, wall.y do
			local w = get(i,j)
			if w == "E" or w == "i" or w == "=" then
				local obj = { i, j }
				wall[wall_n] = obj ; wall_n = wall_n + 1
				gen_wall(i,j,obj)
				if w == "=" then
					obj.type = "door"
				end
				if w == "E" then
					local obj = { i, j }
					wall[wall_n] = obj ; wall_n = wall_n + 1
					gen_external(i,j,obj)
				end
			end
		end
	end
	return wall
end

return map
