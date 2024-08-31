local worker_move = require "gameplay.worker_move"

return function (env)
	env.walk_to_destination = worker_move.walk_to_destination

	function env:moveto_material()
		local container = self.context.container
		local task = self.task
		if container.empty_loot(task.x, task.y) then
			return self.cancel, "NO_PILE"
		end
		return worker_move.move(self)
	end
	function env:pickup()
		local container = self.context.container
		local task = self.task
		local pile = container.get_loot(task.x, task.y)
		if pile then
			-- todo: try all types in the pile
			local count, type = container.check_loot(pile)
			if count then
				container.take_loot(pile, type, count)
				container.pile_put(self.worker.cargo, type, count)
			end
		end
		return self.cont
	end
end
