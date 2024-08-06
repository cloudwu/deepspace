local util = {}

function util.index_map(t, delta)
	delta = delta or 0
	local r = {}
	for i, k in ipairs(t) do
		r[k] = i + delta
	end
	return r
end

function util.keys(t)
	local r = {}
	local n = 1
	for k in pairs(t) do
		r[n] = k; n = n + 1
	end
end

function util.map_from_list(t, func)
	local map = {}
	for _, name in ipairs(t) do
		map[name] = func(name)		
	end
	return map
end

return util