--module setup
local M = {}

-- Import section
local io = io
local print = print

-- Local variables for module-only access (private)

-- No more external access after this point
_ENV = nil

-- Local functions for module-only access

-- in case of early termination it doesn't close the handle
local function iterator(fhandle)
    local _next = fhandle:lines()
    return function(fhandle, j)
	local line = _next()
	if line then return j+1, line
	else fhandle:close() end
    end
end

---------------------------------
-- Public function definitions --
---------------------------------

function M.lines( prog )
    local fhandle = io.popen( prog )
    print('Executing command:', prog)
    return function() return iterator(fhandle), fhandle, 0 end
end

return M

