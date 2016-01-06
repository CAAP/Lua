-- module setup
local M = {}

-- Import Section
local fd = require'carlos.fold'

local flines=io.lines
local open=io.open

local io = io

local tonumber=tonumber
local assert=assert

local st = require'carlos.string'
local tr = require'carlos.transducers'
-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Local function definitions
local function consume(fname, motif, closure)
    local lines = tr.fold()
    function lines.__ipairs() return flines(fname) end
    local function func( a, _, line )
	if #line > 0 and not(line:match('^#')) then a[#a+1] = closure(line:gsub('\r$',''), motif) end
    end
    return lines:reduce( func )
end

-- Function definitions

-- open a file and reads all lines into a table
-- only integers -> '(-?%d+)'
function M.slurp(fname,pattern)
  local p=pattern or "(-?%d+.%d+)" -- numbers
  return consume(fname, p, st.match)
end

function M.dump(fname, astring)
  local file = assert( open(fname,'w') )
  file:write(astring)
  file:close()
  return true
end

function M.dlmread(fname, delimeter)
  local dlm = delimeter or ','
  return consume(fname, dlm, st.split) -- do split; match delimeter
end

function M.readlines(fname)
  return consume(fname, nil, function( line ) return line end)
end

--[[
function M.lines( fname )
    local lines = {}
    function lines.__ipairs() return io.lines( fname ) end
    return lines;
end

function M.lines( fname )
    local file = io.open( fname )
    local function iterator(j, _file) return j+1, _file:lines() end
    return function() return iterator, file, 0 end
end
--]]


function M.lines( fname )
    return fd.wrap( io.lines( fname ) )
end

return M
