-- module setup
local M = {}

-- Import Section
local math = math
local huge = math.huge
local ipairs = ipairs
local tonumber = tonumber

local oo = require'carlos.oo'

-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Local function definitions
local function mapping( a, x, i ) a[#a+1] = a.__f( x, i ) end

local function merging( a, x, k ) if not a[k] then a[k] = x end end

local function filtering( a, x, i ) if a.__f( x, i ) then a[#a + 1] = x end end

local function finding( a, x, i ) if a.__f( x, i ) then a[#a + 1] = { x, i } end end

local function grouping( a, xs, i )
    local k = a.__f( xs, i )
    if not a.keys[k] then local idx = a.idx + 1; a.idx = idx; a.keys[k] = idx; a.lut[idx] = k end
    local idx = a.keys[k]; if not a[idx] then a[idx] = {} end; local w = a[idx]; w[#w + 1] = xs
end

local function stats( a, x )
    if not tonumber(x) then return nil end
-- wiki
    a.n = a.n + 1
    if x < a.min then a.min = x end
    if x > a.max then a.max = x end
    local delta = x - a.avg
    a.avg = a.avg + delta/a.n
    a.ssq = a.ssq + delta*(x - a.avg)
end

local function init( f, v ) local a = v or {}; a.__f = f; return a end

local function inkeys( a ) return function (x) if not a.keys[x] then a.keys[x] = true; return true end end end

local function state(m) if m.__ipairs then return m:__ipairs() else return ipairs(m) end end

local function reduce( m, f, a ) for i,x in state(m) do f( a, x, i ) end return a end

---------------------------------
-- Public function definitions --
---------------------------------

-- J Functional Programming 9 (4): 355-372, July 1999: A tutorial on the universality and expressiveness of fold. Graham Hutton
-- FOLD is an operator that encapsulates a simple pattern of recursion for processing lists. It is defined as:
-- fold f v [] = v | value for empty list
-- fold f v (x : xs) | f x (fold f v xs) | f is applied to first element, and recursively to next elements

M.__index = M

function M.fold( m ) return oo.new( M, m )  end

function M.any( m, f ) for i,x in state(m) do if f(x, i) then return true end end end

function M.first( m, f ) for i,x in state(m) do if f(x, i) then return x, i end end end

function M.reduce( m, f, v ) local a = v or {}; return reduce( m, f, a ) end

function M.map( m, f, v ) local a = init(f, v); return reduce( m, mapping, a ) end

function M.filter( m, f, v ) local a = init(f, v); return reduce( m, filtering, a ) end

function M.groupby( m, f, v ) local a = init(f, v or {keys={}, lut={}, idx=0}); return reduce( m, grouping, a ) end

function M.unique( m, v ) local a =  v or {keys={}}; a.__f = inkeys(a); return reduce( m, filtering, a ) end

function M.findAll( m, f, v ) local a = init(f, v); return reduce( m, finding, a ) end

function M.stats ( m, v ) local a = v or {n=0, min=huge, max=-huge, avg=0, ssq=0}; function a:var() return self.ssq/(self.n-1) end; return reduce( m, stats, a ) end

function M.merge ( m, v) local a = v or {}; return reduce( m, merging, a) end

return M

