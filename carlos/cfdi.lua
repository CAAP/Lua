--module setup
local M = {}

-- Import section
local fd = require'carlos.fold'

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)


-- Local function for module-only access
local function iterate(s)
    assert(s:match'xml', 'Error: Not a valid xml CFDI')
    s:gmatch'<(cfdi:%w+)([^>]+)/?>'
end



---------------------------------
-- Public function definitions --
---------------------------------


return M
