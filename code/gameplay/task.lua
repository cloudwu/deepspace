local task = {}

local func_list_meta = {}

function func_list_meta:__newindex(name, func)
	local list = self._list
	assert(list[name] == nil)
	list[#list+1] = func
	list.name = func
end

local task_meta = {}; task_meta.__index = task_meta

local TASK_CONTINUE <const> = 0
local TASK_YIELD <const> = 1
local TASK_CANCEL <const> = 2

local task_instance_meta = {
	cont = TASK_CONTINUE,
	yield = TASK_YIELD,
	cancel = TASK_CANCEL,
}; task_instance_meta.__index = task_instance_meta

function task_instance_meta:update()
	local idx = self._current
	local func_list = self._list
	local func = assert(func_list[idx])
	local ret, err = func(self)
	if ret == TASK_CONTINUE then
		idx = idx + 1
		self._current = idx
		if idx > #func_list then
			-- task done
			return false
		end
		-- task continue
		return self:update()
	elseif ret == TASK_YIELD then
		-- task yield
		return true
	end
	assert(ret == TASK_CANCEL)
	return nil, err
end

function task_instance_meta:checkpoint()
	self._checkpoint = self._current
end

function task_instance_meta:reset(cp)
	local c = self._checkpoint or cp or 1
	self._current = c
	return c
end

function task_meta:instance(init)
	init = init or {}
	init._list = self
	init._current = 1
	return setmetatable(init, task_instance_meta)
end

function task.define(func)
	local func_list = {}
	local setter = setmetatable({ _list = func_list }, func_list_meta)
	func(setter)
	return setmetatable(func_list, task_meta)
end

return task
