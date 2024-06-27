local coroutine = import_package "ant.coroutine"

return function (world)
	local async = {}; async.__index = async
	
	local function async_create_instance(self, arg)
		function arg.on_ready(inst)
			local r = self._ready
			if r >= self._wait then
				self._wait = 0
				self._ready = 1
				assert(coroutine.resume(self._co))
			else
				self._ready = r + 1
			end
		end
		self._wait = self._wait + 1
		return world:create_instance(arg)
	end
	
	function async:create_instance(arg)
		local inst = async_create_instance(self, arg)
		coroutine.yield()
		return inst
	end
	
	async.async_instance = async_create_instance
	async.wait = coroutine.yield

	return function (main_func)
		local co = coroutine.create(main_func)
		local async_inst = setmetatable( { _co = co, _wait = 0, _ready = 1 }, async )
		assert(coroutine.resume(co, async_inst))
	end
end
