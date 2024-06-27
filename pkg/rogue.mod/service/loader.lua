local lfs = require "bee.filesystem"
local memfs = import_package "ant.vfs".memory

memfs.init()

local args = ...

local function load_dir(local_path, vfs_path)
	for path, attr in lfs.pairs(local_path) do
		if attr:is_directory() then
			load_dir(path, vfs_path .. "/" .. path:filename():string())
		else
			print(vfs_path .. "/" .. path:filename():string() .. " <- " .. path:string())
			memfs.update(vfs_path .. "/" .. path:filename():string(), path:string())
		end
	end
end

local function load(modpath)
	if lfs.exists (modpath) then
		load_dir(modpath, "")
	end
end

local startup = args[0]
local modpath
if startup then
	modpath = lfs.path(startup):parent_path() / "mod"
else
	modpath = lfs.current_path() / "mod"
end

load(modpath)
