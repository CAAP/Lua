-- module setup
local M = {}

-- Import Section

-- Local Variables for module-only access

-- d3.layout.histogram() -> func()
-- constructs a new histogram function which by default returns frequencies

local function histogram()
end

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

return M
