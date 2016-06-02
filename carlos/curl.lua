--module setup
local M = {}

-- Import section
local lcurl = require'lcurl'

local table = table

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

-- Local function for module-only access

---------------------------------
-- Public function definitions --
---------------------------------

function M.get( url )
    local ret = lcurl.get( url )
    return table.concat( ret )
end

return M
