local debuginfo = {}

local meta = {
	__index = function (o, k)
		local r = {}
		o[k] = r
		return r
	end
}

local queue = setmetatable({}, meta)

function debuginfo.add(what, id, text)
	queue[what][id] = text
end

function debuginfo.update(message)
	local n = #message
	local start = n
	for k, set in pairs(queue) do
		for id, text in pairs(set) do
			n = n + 1
			message[n] = { what = k, action = "info", id = id, text = text }
		end
	end
	if start ~= n then
		queue = setmetatable({}, meta)
	end
end

return debuginfo