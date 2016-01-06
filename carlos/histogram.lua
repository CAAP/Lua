-- module setup
local M = {}

-- Import Section
local assert = assert
local huge = math.huge

local fd = require'carlos.fold'

local math = math
local tr = require'carlos.transducers'
local ops = require'carlos.operations'
local sort = table.sort

-- Local Variables for module-only access

-- Local function definition
local function round(x)
    return math.floor( x * 1e5 + 0.5 )/1e5
end

local function stats( a, q, x )
-- wiki
    if q == 0 then return nil end
    a.n = a.n + q
    if x < a.min then a.min = x end
    if x > a.max then a.max = x end
    local delta = x - a.avg
    a.avg = a.avg + delta * q / a.n
    a.ssq = a.ssq + delta * (x - a.avg) * q
end

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

function M.robust( keys, counts, thr )
    assert( #keys == #counts, 'Number of keys and values must be equal.' )
    local w = thr or 0.005
    local iw = 1-w
    local sum = fd.reduce( counts, fd.sum, {} ).sum
    local mni = ops.bsearch( #counts, function(j) return round(counts[j]/sum) < w end )
    local mxi = fd.first( ops.range(#counts,1,-1), function(j) return round(counts[j]/sum) >= w end )
    local ks, cts = {}, {}
    for j = mni, mxi do ks[#ks+1] = keys[j]; cts[#cts+1] = counts[j] end
    return ks, cts
end

function M.stats( keys, counts )
    if counts then
	local a = tr.reduce( keys, function(b,x,i) stats(b, counts[i], x) end, {n=0, min=huge, max=-huge, avg=0, ssq=0} )
	a.var = a.ssq / (a.n - 1)
	a.stdev = math.sqrt( a.var )
	return a
    else
	local a = tr.reduce( keys, stats, {n=0, min=huge, max=-huge, avg=0, ssq=0} )
        a.var = a.ssq / (a.n - 1)
	a.stdev = math.sqrt( a.var )
	return a
    end
end

function M.sample( keys, counts, n )
    local cum = tr.reduce( counts, function(a, q) a.sum = a.sum + q; a[#a+1] = a.sum end, {sum=0} )
    local sum = cum.sum
    for i=1,#cum do cum[i] = round(cum[i]/sum) end
    local ret = {}
    for i=1,n do
	local r = math.random()
	local idx = ops.bsearch( #cum, function(j) return cum[j] < r end )
	ret[i] = keys[idx]
    end
    return ret
end

function M.histogram( data, bins )
    sort(data)
    local stats = tr.stats( data )
    local ks = ops.range( ops.ticks( stats, bins ) )
    local ret = tr.map( ks, function() return 0 end ) -- init count to 0
    tr.reduce( data, function(a, x) local kk = ops.bsearch( #ks, function(i) return ks[i] < x end); a[kk] = a[kk] + 1 end, ret )
    return ret, ks
end

return M

