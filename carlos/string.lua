-- module setup
local M = {}

-- Import Section
local ipairs=ipairs
local tonumber=tonumber
local tostring=tostring
local os = os
local io = io

local string=string

local tr=require'carlos.transducers'

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

-- Local function definitions

local function iter(dlm, s1)
    local i,j = s1:find(dlm, 1, true)
    if i then return s1:sub(j+1) , s1:sub(1, i-1)
    else return #s1>0 and '' or nil, s1 end
end

local function nicetime( s )
    local l = 'sec(s)'
    if s > 60 then s = s / 60; l = 'min(s)'
	if s > 60 then s = s / 60; l = 'hr(s)'
	    if s > 24 then s = s / 24; l = 'day(s)' end
	end
    end
    return l:sub(1,1) == 's' and string.format('%5.0f %s', s, l) or string.format('%5.2f %s', s, l)
end

--------------------------
-- Function definitions --
--------------------------

function M.status( size )
    local remaining = '\r%2.0f%% done, %s remaining'
    local start = os.time()
    local last = start
    return function( step )
	return function(x, i)
	    local current = os.time()
	    if current - last > 1 then
	        local rem = (size - i) * (current - start) / i
	        io.stderr:write( string.format( remaining, 100 * i / size, nicetime( rem ) ) )
	        io.stderr:flush()
	        last = current
	    end
	    step(x, i)
	end
    end
end

function M.split(s, dlm, folded)
    local ss = tr.fold()
    function ss.__ipairs() return iter, dlm, s end
    if folded then return ss
    else
	local ans = ss:map( function(x) return tonumber(x) or x end )
	if #ans == 1 and tostring(ans[1]):match('^%s*$') then ans = nil end
	return ans
    end
end

function M.match(s, pattern, folded)
    local ss = tr.fold()
    function ss.__ipairs() return s:gmatch(pattern) end
    if folded then return s
    else return tr:map( function(_,x) return tonumber(x) or x end ) end
end

return M
