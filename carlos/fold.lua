-- module setup
local M = {}

-- Import Section
local assert = assert
local remove = table.remove
local huge = math.huge
local ipairs = ipairs
local pairs = pairs
local tonumber = tonumber
local type = type

local print = print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

-- J Functional Programming 9 (4): 355-372, July 1999: A tutorial on the universality and expressiveness of fold. Graham Hutton
-- FOLD is an operator that encapsulates a simple pattern of recursion for processing lists. It is defined as:
-- fold f v [] = v | value for empty list
-- fold f v (x : xs) | f x (fold f v xs) | f is applied to first element, and recursively to next elements

-- Lua Reference Manual
-- for var_1, ..., var_n in explist do block end
-- do
--   local f, s, var = explist
--   while true do
--     local var_1, ..., var_n = f(s, var)
--     if var_1 == nil then break end
--     var = var_1
--     block
--   end
-- end
-- explist is evaluted only once, and results in an iterator function, a state and an initial value for the first iterator variable

--------------------------------
-- Local function definitions --
--------------------------------

local function stats( a )
    local function acc(x)
	if tonumber(x) then
	    a.n = a.n + 1
    	    if x < a.min then a.min = x end
    	    if x > a.max then a.max = x end
	    local delta = x - a.avg
	    a.avg = a.avg + delta/a.n
	    a.ssq = a.ssq + delta*(x - a.avg)
	end
    end
    a.n = a.n or 0; a.min = a.min or huge; a.max = a.max or -huge; a.avg = a.avg or 0; a.ssq = a.ssq or 0;
    return acc
end

local function _f(_next, j) local ret = _next(); if ret then return j+1, ret end end

local function state( m )
    if type(m) == 'table' then return ipairs(m)
    elseif type(m) == 'function' then return m()
    else print('I do not know how to reduce a',type(m)) end
end

-- becuse it becomes an upvalue of the closure, do not modify the upvalue; alt: debug.getupvalue & debug.setupvalue
local function buffer() local m={slice={}}; return function(x) m.slice[#m.slice+1] = x end, m end

local function fcomp( fns )
    local N = #fns
    assert( N > 0, 'There must be at least one argument.' )
    local fx = fns[N]
    for j = N-1,1,-1 do fx = fns[j]( fx ) end
    return fx
end
--[[
local function comp( args )
    local N = #args
    assert( N > 0, 'There must be at least one argument.' )
    if N == 1 then assert( type(args[1]) == 'function', 'One argument given, it must be a function.'); return args[1], nil end
    if N == 2 then assert( type(args[2]) ~= 'function', 'Last argument cannot be a function.'); return args[1]( args[2] ), args[2] end -- or can it be?
    local fx = fcomp( args )
    return fx, args[N]
end
--]]
local function comp( args )
    local N = #args
    assert( N > 0, 'There must be at least one argument.' )
    if N == 1 then assert( type(args[1]) == 'function', 'One argument given, it must be a function.'); return args[1], nil end
    local fx = fcomp( args )
    return fx, ((type(args[N]) ~= 'function') and args[N] or nil)
end

local function xcomp( args )
    local N = #args
    assert( N > 0, 'There must be at least one argument.' )
    if N == 1 then assert( type(args[1]) == 'function', 'One argument given, it must be a function.'); return args[1], nil, buffer() end
    if N == 2 then return args[1]( args[2] ), args[2], buffer() end
    local a = remove( args ); local g = remove( args )( a )
    local f, b = buffer()
    args[N-1] = f
    local fx = fcomp( args )
    return g, a, fx, b
end

---------------------------------
-- Public function definitions --
---------------------------------

-- aux --

M.comp = fcomp

function M.wrap( iter ) return function() return _f, iter, 0 end end

function M.keys( t ) return function() return pairs(t) end end

function M.flatten( f ) return function(a) local ff = f(a); return function(x, k) if type(x) == 'table' then for w,y in pairs(x) do ff( y, w ) end else ff( x, k ) end end end end

function M.apply( ... )
    local fns = {...}
    local N = #fns
    assert( N > 0, 'There must be at least one argument.' )
    local fx = fns[1]
    for j = 2,N do fx = fns[j]( fx ) end
    return fx
end

-- loops --

function M.reduce( m, ... ) local _f, _a = comp{...}; for i,x in state(m) do _f(x, i) end; return _a end
 
function M.take( k, m, ... ) local _f, _a = comp{...}; for i,x in state(m) do _f(x, i); if i == k then break end end; return _a end

function M.conditional( Cf, m, ... ) local _f, _a = comp{...}; for i,x in state(m) do if Cf(x,i) then break else _f(x, i) end end; return _a end

function M.drop( k, m, ... ) local _f, _a = comp{...}; for i,x in state(m) do if i > k then _f(x, i) end end; return _a end

function M.times(k, ... ) local _f, _a = comp{...}; for i=1,k do _f(i) end; return _a end

function M.any( m, ... ) local _f = fcomp{...}; for i,x in state(m) do if _f(x, i) then return true end end end

-- all := not(any)
--function M.all( m, ... ) local _f = fcomp{...}; for i,x in state(m) do if not(_f(x, i)) then return true end end end

function M.first( m, ... ) local _f = fcomp{...}; for i,x in state(m) do if _f(x, i) then return x, i end end end

function M.slice( k, m, ... )
    local _g, _a, _f, _b = xcomp{...}
    for i,x in state(m) do _f(x, i); if #_b.slice == k then _g( _b.slice, i ); _b.slice =  {} end end
    if #_b.slice > 0 then _g( _b.slice, 0 ) end -- 0 place holder
    return _a
end

M.steps = M.slice

 -- accumulators --

M.stats = stats

function M.into( a ) return function(x) a[#a+1] = x end end

function M.merge( a ) return function(x, k) if not a[k] then a[k] = x end end end

function M.statsBy( a )
    local lstats = {}
    local function acc(x)
	if tonumber(x.value) then
	    local k = x.group
	    if not lstats[k] then a[#a+1] = {}; lstats[k] = stats(a[#a]); a.keys[#a] = k end
	    lstats[k](x.value)
	end
    end
    a.keys = a.keys or {}
    return acc
end

function M.max( a ) a.max = a.max or -huge return function( x, i ) if x > a.max then a.max = x; a.idx = i end end end

function M.min( a ) a.min = a.min or huge return function( x, i ) if x < a.min then a.min = x; a.idx = i  end end end

function M.sum( a ) a.sum = a.sum or 0 return function( x, i ) a.sum = a.sum + x end end

 -- connectors --

function M.rejig( f ) return function(step) return function( x, i ) step( f(x, i) ) end end end

function M.map( f ) return function(step) return function( x, i ) step( f(x, i), i ) end end end

function M.filter( f ) return function(step) return function( x, i ) if f(x, i) then step( x, i) end end end end

function M.unique( a ) return function(step) local keys = a or {}; return function( x, i ) if not keys[x] then keys[x] = true; step( x, i ) end end end end

function M.uniqueBy( f ) return function(step) local ks = {}; return function( x, i ) local k = f(x,i); if not ks[k] then ks[k] = true; step( x, k ); end end end end

function M.sliceBy( f ) return function(step) local b = {}; return function( x, i ) b[#b+1] = x; if f(x, i) then step(b, i); b = {} end end end end

return M

