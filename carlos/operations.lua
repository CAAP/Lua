-- module setup
local M = {}

-- Import Section
local math = math
local tr = require'carlos.transducers'

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

-- Local function defitions
-- < x | y >
local function dot( x,y ) return x[1]*y[1] + x[2]*y[2] + x[3]*y[3] end
 
-- < a1 x a2 | b1 x b2 > = <a1|b1> <a2|b2> - <a2|b1> <a1|b2>
local function crossdot(m, n)
    local a,b,c,d = 0, 0, 0, 0
    for i=1,3 do a = a + m[i]*n[i]; b = b + m[i+3]*n[i+3]; c = c + m[i]*n[i+3]; d = d + m[i+3]*n[i] end
    return a*b - c*d
end

-- cumulative distribution function (unnormalized)
local function cdf( h )
    local acc = 0
    return tr.map( h, function(x) acc = acc + x; return acc end )
end

local function bsearch( n, f )
    local i,j = 1,n
    while i < j do
	local h = i + math.floor((j - i)/2)
	if f(h) then i = h + 1 else j = h end
    end
    return i
end

local function hquantile( cdf, p )
    local N = cdf[#cdf]
    local H = (N-1)*p + 1
    local h = math.floor( H )
    local e = H - h
    if e == 0 then return xs[h] end
    local d = xs[h+1] - xs[h]
    if d == 0 then return xs[h] else return xs[h] + e*d end
end

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

-- binary search algorithm; only works for orderded data
M.bsearch = bsearch

-- Quantile function is defined for real variables [0,1] and is mathematically the inverse of the cumulative distribution function (CDF).
-- The k-th quantile is the data value where the cumulative distribution crosses k/q -> Pr[X < x] = k/q.
-- Instead of using k and q, the p-quantile is based on a real number with 0 < p < 1. Then, p replaces k/q.
-- For a finite population of N values indexed from lowest to highest, the k-th quantile can be computed as Index[X] = N*k/q
-- wikipedia: R-7
-- h := (N-1)*p + 1
-- _h_ := floor(h)
-- Q := x[_h_] + (h - _h_)*(x[_h_ + 1] - x[_h_])
function M.quantile( xs, p )
    local N = #xs
    local H = (N-1)*p + 1
    local h = math.floor( H )
    local e = H - h
    if e == 0 then return xs[h] end
    local d = xs[h+1] - xs[h]
    if d == 0 then return xs[h] else return xs[h] + e*d end
end

function M.range(start, stop, step)
    local step = step or 1
    local k = 1
    local delta = math.abs(step)
    while delta*k%1 ~= 0 do k = k*10 end
    local ret = {}
    for i = start*k, stop*k, step*k do ret[#ret+1] = i/k end
    ret.step = step
    return ret
end

function M.last( m, f )
    local y,j = tr.first( m, f )
    if y then
	for i=j+1,#m do
	    if not f(m[i], i) then return m[i-1], i-1 end
	end
	return m[#m], #m
    end
end

function M.choose(n, k)
    if k < 0 or k > n then return 0 end
    if k == 0 or k == n then return 1 end
    local k = math.min(k, n-k)
    local c = 1
    for i = 0, k-1 do
	c = c * (n - i) / (i + 1)
    end
    return c
end

function M.factorial( n )
    local ret = 1
    for i=1, n do ret = ret * i end
    return ret
end

function M.ticks(aggregates, bins)
    local min, max = math.floor(aggregates.min), math.ceil(aggregates.max)
    local span = max - min
    local n = bins or 10
    local step = math.pow(10, math.floor( math.log(span/n, 10) ))
    local err = n / span * step

    -- get closer to desired count
    if err <= 0.15 then step = step * 10
    elseif err <= 0.35 then step = step * 5
    elseif err <= 0.75 then step = step * 2 end

    -- round start and stop to step interval
    local start = math.ceil( min / step ) * step
    local stop = math.floor( max / step ) * step + 0.5 * step
    return start-2*step, stop+2*step, step
end

--[[
local function byTicks(xs, bins)
    sort(xs)
    local stats = fold( xs ).stats()
    local ks = range( ticks( stats, bins ) )
    local ret = fold( ks ).map( function(k) return {} end)
    fold( xs ).groupby(function(x) return ops.bsearch(#ks, function(i) return ks[i] < x end) end, ret)
    return ret, ks
end
--]]

return M
