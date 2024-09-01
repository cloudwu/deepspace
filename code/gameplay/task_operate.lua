local worker_move = require "gameplay.worker_move"

return function (env)
	env.walk_to_destination = worker_move.walk_to_destination
	env.moveto_machine = worker_move.move
	function env:start()
		self:checkpoint()
		local machine = self.context.machine
		-- todo: worker have different productivity (not 100%)
		machine.start(self.task.machine, 1)
		return self.cont
	end
	function env:working()
		-- todo: check power
		local machine = self.context.machine
		-- todo: work Interrupted  (Hungry?)
		if machine.finish(self.task.machine) then
			return self.cont
		else
			return self.yield
		end
	end
end
