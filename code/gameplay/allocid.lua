return function ()
	local id = 0
	return function (init_id)
		if init_id then
			if init_id > id then
				id = init_id
			end
			return init_id
		else
			id = id + 1
			return id
		end
	end
end