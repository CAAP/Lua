--module setup
local M = {}

-- Import section
local open = io.open
local fd = require'carlos.fold'

-- No more external access after this point
_ENV = nil


-- Local variables for module-only access (private)


-- Local function for module-only access

local function astable(path)
    local f = open(path)
    local d = f:read'a'
    f:close()
    local ret = {}
    for k,v in d:gmatch'</?cfdi:(%w+)([^>]*)>' do
	ret[#ret+1] = {k, #v > 0 and v or nil}
    end
    return ret
end


---------------------------------
-- Public function definitions --
---------------------------------


return M
