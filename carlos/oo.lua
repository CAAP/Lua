--module setup
local M = {}

-- Import section
local setmetatable = setmetatable

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

-- Local function for module-only access

---------------------------------
-- Public function definitions --
---------------------------------

function M.new( self, a )
    local o = a or {}
    setmetatable( o, self )
    self.__index = self
    return o
end

return M
