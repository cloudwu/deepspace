local serialize = import_package "ant.serialize"
local datalist = require "datalist"
local lfs  = require "bee.filesystem"
local util = require "util"

local loadsave = {}

local save = {}; save.__index = save

function save:open(filename)
	if self._file then
		self._file:close()
	end
	self._filename = filename
	local f , err = io.open(filename .. ".tmp", "wb")
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
		local filename = self._filename
		if lfs.exists(filename) then
			lfs.rename(filename, filename .. ".bak")
			lfs.rename(filename .. ".tmp", filename)
			lfs.remove(filename .. ".bak")
		else
			lfs.rename(filename .. ".tmp", filename)
		end
	end
end

save.__close = save.close

local function tolist(v)
	local t = {}
	for i = 1, #v do
		local item = v[i]
		if type(item) == "string" then
			t[i] = datalist.quote(item)
		else
			t[i] = tostring(item)
		end
	end
	return "{ " .. table.concat(t, ",") .. " }"
end

local function write_kv(f, t, ident)
	local keys = util.keys(t)
	table.sort(keys)
	for i = 1, #keys do	
		local k = keys[i]
		local v = t[k]
		local t = type(v)
		if t == "string" then
			v = datalist.quote(v)
		elseif t == "table" then
			v = tolist(v)
		else
			v = tostring(v)
		end
		f:write(ident, k, ":", v, "\n")
	end
end

function save:write_object(key, object)
	self._file:write(key, ":\n")
	write_kv(self._file, object, "\t")
end

function save:write_list(key, list)
	self._file:write(key, ":\n")
	for _, item in ipairs(list) do
		self._file:write("\t---\n")
		write_kv(self._file, item, "\t")
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
