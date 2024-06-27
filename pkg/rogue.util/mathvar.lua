local math3d = require "math3d"

local mark = math3d.mark
local unmark = math3d.unmark

local conv_func <const> = {
	mat = math3d.matrix,
	v4 = math3d.vector,
	quat = math3d.quaternion,
}

return function(struct)
	local conv = {}
	local raw = {}
	for k,v in pairs(struct) do
		local t = math3d.totable(v).type
		conv[k] = conv_func[t] or error ("Invalid math3d type " .. t)
		raw[k] = mark(v)
	end
	
	local function accessor(o, key, v)
		local mv = mark(conv[key](v))
		unmark(raw[key])
		raw[key] = mv
	end
	
	local function unmark_var(o)
		for k,v in pairs(raw) do
			unmark(v)
			raw[k] = nil
		end
	end
	
	local function tostring_var(o)
		local s = { "[" }
		local n = 2
		for k,v in pairs(raw) do
			s[n] = ("%s:%s"):format(k, math3d.tostring(v)); n = n + 1			
		end
		s[n] = "]"
		return table.concat(s, " ")
	end
	
	local meta = {
		__index = raw,
		__newindex = accessor,
		__gc = unmark_var,
		__tostring = tostring_var,
	}
	return setmetatable({} , meta)
end
