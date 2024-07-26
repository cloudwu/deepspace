local serialize = import_package "ant.serialize"

local config = serialize.load(localpath "config.ant")

return config
