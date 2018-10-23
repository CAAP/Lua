-- module setup
local M = {}

-- Import Section
local bsd = require'lbsd'

local assert = assert

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

--------------------------------
-- Local function definitions --
--------------------------------

---------------------------------
-- Public function definitions --
---------------------------------

function M.unveil(path, flags)
    assert(path and not(path:match'^%s+$'), "At leat one argument given: path to unveil.")
    return bsd.unveil(path, flags or 'rwxc')
end

return M
