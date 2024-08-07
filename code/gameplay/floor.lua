return function(scene)
	local floor = {}
	
	local cap_x = scene.x
	local cap_y = scene.y

	local function clamp(x, y)
		if x < 0 then
			x = 0
		elseif x >= cap_x then
			x = cap_x - 1
		end
		if y < 0 then
			y = 0
		elseif y >= cap_y then
			y = cap_y - 1
		end
		return x, y
	end

	local function range(x1, y1, x2, y2)
		x1, y1 = clamp(x1, y1)
		x2, y2 = clamp(x2, y2)
		if x1 > x2 then
			x1, x2 = x2, x1
		end
		if y1 > y2 then
			y1, y2 = y2, y1
		end
		return x1, y1, x2, y2
	end
	
	local change = {}
	
	function floor.add(x1, y1, x2, y2)
		x1, y1, x2, y2 = range(x1, y1, x2, y2)
		for x = x1, x2 do
			for y = y1, y2 do
				if scene.set_floor(x, y, true) then
					local index = x << 16 | y
					change[index] = true
				end
			end
		end
	end

	function floor.remove(x1, y1, x2, y2)
		x1, y1, x2, y2 = range(x1, y1, x2, y2)
		for x = x1, x2 do
			for y = y1, y2 do
				if scene.set_floor(x, y, false) then
					local index = x << 16 | y
					change[index] = false
				end
			end
		end
	end
	
	function floor.update(message)
		if next(change) then
			local n = #message
			for index, build in pairs(change) do
				n = n + 1
				message[n] = { what = "floor", action = build and "build" or "remove",
					x = index >> 16, y = index & 0xffff }
			end
			message[n+1] = { what = "floor", action = "change" }
			change = {}
		end
	end
	
	local save_key <const> = "floor"

	function floor.export(file)
		local data, x, y, w, h = scene.export_floor()
		if not data then
			return
		end
		local info = {
			x = x,
			y = y,
			w = w,
			h = h,
			data = data,
		}
		file:write_object(save_key, info)
	end
	
	function floor.clear()
		scene.clear_floor()
	end

	function floor.import(savedata)
		local obj = savedata[save_key]
		if not obj then
			return
		end
		local n = 1
		local data = obj.data
		local dx = obj.x
		local dy = obj.y
		local unpack = string.unpack
		local set_floor = scene.set_floor
		for y = 0, obj.h - 1 do
			for x = 0, obj.w - 1 do
				local enable = unpack("<I4", data, n)
				local xx = x + dx
				local yy = y + dy
				if set_floor(xx, yy, enable ~= 0) then
					local index = xx << 16 | yy
					change[index] = true
				end
				n = n + 4
			end
		end
	end

	return floor
end
