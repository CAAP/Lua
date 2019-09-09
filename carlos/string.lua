-- module setup
local M = {}

-- Import Section
local ipairs=ipairs
local pairs=pairs
local tonumber=tonumber
local tostring=tostring
local os = os
local io = io
local utf8 = utf8

local string=string

local tr=require'carlos.transducers'

local fd = require'carlos.fold'

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
    local ss = function() return iter, dlm, s end

    if folded then return ss
    else
	local ans = fd.reduce(ss, fd.map(function(x) return tonumber(x) or x end), fd.into, {})
	if #ans == 1 and tostring(ans[1]):match('^%s*$') then ans = nil end
	return ans
    end
end

--[[
function M.match(s, pattern, folded)
    local ss = tr.fold()
    function ss.__ipairs() return s:gmatch(pattern), nil, nil end
    if folded then return ss
    else return tr:map( function(_,x) return tonumber(x) or x end ) end
end
--]]

function M.bigram(s1, s2)
    local set, all, both, q, p = {}, {}, {}, 0, 0

    for j=2,#s1 do local ss = s1:sub(j-1,j); all[ss] = true; set[ss] = true; end

    for j=2,#s2 do local ss = s2:sub(j-1,j); if set[ss] then both[ss] = true else all[ss] = true end end

    for _ in pairs(all) do p = p + 1 end

    for _ in pairs(both) do q = q + 1 end

    return q / p
end

function M.jaccard(s1, s2)
    local set, all, both, q, p = {}, {}, {}, 0, 0

    for _,j in utf8.codes(s1) do all[j] = true; set[j] = true; end

    for _,j in utf8.codes(s2) do if set[j] then both[j] = true else all[j] = true end end

    for _ in pairs(all) do p = p + 1 end

    for _ in pairs(both) do q = q + 1 end

    return q / p
end

return M
