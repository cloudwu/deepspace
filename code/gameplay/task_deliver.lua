local worker_move = require "gameplay.worker_move"

return function (env)
	env.walk_to_destination = worker_move.walk_to_destination

	function env:moveto_loot()
		local container = self.context.container
		local task = self.task
		if not container.pile_stock(task.pile) then
			return "finish"
		end
		return worker_move.move(self)
	end
	function env:pickup()
		local container = self.context.container
		local task = self.task
		local count, type = container.pile_stock(task.pile)
		if not count then
			return "finish"
		end
		container.pile_take(task.pile, type, count)
		container.pile_put(self.worker.cargo, type, count)
		return self.cont
	end
	function env:finish()
		return self.cont
	end
end
