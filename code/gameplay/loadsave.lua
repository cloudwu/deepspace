local serialize = import_package "ant.serialize"
local datalist = require "datalist"
local lfs       = require "bee.filesystem"

local loadsave = {}

local save = {}; save.__index = save

function save:open(filename)
	if self._file then
		self._file:close()
	end
	local f , err = io.open(filename, "wb")
	if not f then
		return nil, err
	end
	self._file = f
	return true
end

function save:close()
	if self._file then
		self._file:close()
		self._file = nil
	end
end

save.__close = save.close

function save:write_object(key, object)
	self._file:write(key, " :\n")
	for k,v in pairs(object) do
		if type(v) == "string" then
			v = datalist.quote(v)
		else
			v = tostring(v)
		end
		self._file:write("  ", k, " : ", v, "\n")
	end
end

function loadsave.savefile(filename)
	local obj = setmetatable({} , save)	
	if filename then
		assert(obj:open(filename))
	end
	return obj
end

function loadsave.load(filename)
	if lfs.exists(filename) then
		return serialize.load_lfs(filename)
	end
end

return loadsave
