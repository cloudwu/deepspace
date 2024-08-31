local worker_move = require "gameplay.worker_move"

return function (env)
	env.walk_to_destination = worker_move.walk_to_destination
	env.moveto_blueprint = worker_move.move
	function env:build_start()
		self:checkpoint()
		local machine = self.context.machine
		local blueprint = self.task.blueprint
		-- todo: worker have different productivity (not 100%)
		machine.start(blueprint, 1)
		return self.cont
	end
	function env:building()
		local machine = self.context.machine
		local blueprint = self.task.blueprint
		-- todo: work Interrupted  (Hungry?)
		if machine.finish(blueprint) then
			return self.cont
		else
			return self.yield
		end
	end
end
