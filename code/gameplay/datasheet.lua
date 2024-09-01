local serialize = import_package "ant.serialize"
local fs = require "filesystem"

local data = {}

local function merge_sheet(root, k, v, path)
	local old = root[k]
	if old == nil then
		root[k] = v
		return
	end
	if type(v) ~= "table" or type(old) ~= "table" then
		error ("Can't merge " .. path .. "/" .. k)
	end
	path = path .. "/" .. k
	for k,v in pairs(v) do
		merge_sheet(old, k, v, path)
	end
end

local function add_file(filename)
	local sheet = serialize.load(filename)
	for k,v in pairs(sheet) do
		merge_sheet(data, k, v, "")
	end
end

local function add_dir(root)
	for path, attr in fs.pairs(root) do
		if attr:is_directory() then
			add_dir(path)
		elseif path:extension() == ".ant" then
			add_file(path:string())
		end
	end
end

add_dir "/datasheet"

local material_idmap = {}
local material_stack = {}

local function arrange_material(mat)
	local idlist = {}
	for name,v in pairs(mat) do
		local id = v.id
		v.name = name
		material_idmap[name] = id
		idlist[id] = v
		material_stack[id] = v.stack
	end
	return idlist
end

local function material_id(mat, map)
	if not mat then
		return
	end
	local idlist = {}
	local n = 1
	for name, v in pairs(mat) do
		idlist[n] = {
			id = map[name],
			count = v
		}
		n = n + 1
	end
	return idlist
end

local building_idmap = {}
local function arrange_buildings(building, mat, recipe)
	local idlist = {}
	for name, v in pairs(building) do
		local id = v.id
		building_idmap[name] = id
		v.name = name
		v.material = material_id(v.material, mat)
		if v.recipe then
			v.recipe = recipe[v.recipe] or error ("Unknown recipe " .. tostring(v.recipe))
		end
		idlist[id] = v
	end
	return idlist
end

local function building_time(buildings, data)
	data = data or {}
	for k,v in pairs(buildings) do
		data[k] = { worktime = v.building_time }
	end
	return data
end

local function arrange_pair(p, idmap)
	if not p then
		return
	end
	local r = {}
	for k,v in pairs(p) do
		k = idmap[k] or ("Unknown " .. tostring(k))
		r[k] = v
	end
	return r
end

local recipe_idmap = {}
local function arrange_recipe(recipe, idmap)
	local r = {}
	for k,v in pairs(recipe) do
		v.name = k
		v.input = arrange_pair(v.input, idmap)
		v.output = arrange_pair(v.output, idmap)
		r[v.id] = v
		recipe_idmap[k] = v.id
	end
	return r
end

data.material = arrange_material(data.material)
data.material_id = material_idmap
data.material_stack = material_stack
data.recipe = arrange_recipe(data.recipe, material_idmap)
data.recipe_id = recipe_idmap
data.building = arrange_buildings(data.building, material_idmap, recipe_idmap)
data.building_id = building_idmap
data.blueprint = building_time(data.building)

return data
