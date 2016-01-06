-- module setup
local M = {}

-- Import Section
local fd = require'carlos.fold'

local math = math

-- Local Variables for module-only access

local huge = math.huge

-- No more external access after this point
_ENV = nil -- or M

--------------------------------
-- Local function definitions --
--------------------------------

---------------------------------
-- Public function definitions --
---------------------------------

local function measure( closure )
    return function( x, y )
	local sum = 0.0
	fd.reduce( x, function(q, k) if y[k] then sum = sum + closure( q, y[k], k)  end end )
    	return sum
    end
end

---------------------------------------------------------
-- tolearance allows removal of very 'dis-similar' points
-- a metric that is symmetric avoids calculation of s_ji
---------------------------------------------------------

M.measure = measure

M.euclidean = measure( function(x, y) return math.pow( x-y, 2 ) end )

function M.similarity(points, metric, symmetric, tolerance)
    local size = #points
    local sims = {}
    local sij, sji
    local tol = tolerance or -huge
    
    for i=1,size do
	for j=i+1,size do
	    sij = metric(points[i], points[j])
	    sji = symmetric and sij or metric(points[j], points[i])
	    sims[#sims+1] = sij>tol and {i, j, sij} or nil
	    sims[#sims+1] = sji>tol and {j, i, sji} or nil
	end
    end
    
    return sims
end

return M
--[[

M.euclidean = metric( function(x,y) return math.pow(x-y,2) end, true, false )
M.taxicab = metric( function(x,y) return math.abs(x-y) end, true, false )
M.bintersection = metric( function() return 1 end, false, true, function(x) return #x end)
M.intersection = metric( math.min, false, true, function(x) local sum=0; fd.reduce( x, function(q) sum = sum+q end); return sum end )
M.divergence = metric( function(x,y) return math.abs(math.log(x/y)*x) end, false, false ) -- Kullback-Leibler divergence; values run from (-inf, inf); necessary to take abs
M.chisqr = metric(function(x,y) return (x+y) == 0 and 0 or math.pow(x-y,2)/(x+y) end, true, false ) -- Chi-squared distance

-- input is a distance-based metric --
local function tokernel(ametric, kernel)
    assert(ametric.distance, 'A distance-based metric is required.')
    local MM = {}
    local k = kernel or korn.exponential
    local distance = ametric.distance

    MM.symmetry = ametric.symmetry 
    MM.similarity = function(x,y) return k(distance(x,y)) end

    return MM
end


---------------------------------
-- Kolmogorov-Smirnov distance --
-- input: cumulative histogram --
---------------------------------
do
    local MM = {}
    MM.simmetry = true

    function MM.distance(x,y)
        local mx  = -1
	for i,v in ipairs(x) do
	    local d = abs(v - y[i])
	    if d > mx then mx = d end
	    if mx == 1 then break end -- for normalized histograms 1 is the max distance between distributions
	end

	return mx
    end

    M.kolmogorov = MM
end

-- KERNELS --
M.gaussian = tokernel(M.euclidean)
M.laplacian = tokernel(M.taxicab)
M.kdiv = tokernel(M.divergence)
M.kchi = tokernel(M.chisqr)
M.kks = tokernel(M.kolmogorov)

M.tokernel = tokernel
--[[
function M.gaussian(kwidth, weight)
    local hh = kwidth or 1
    local ws = weight or 1

    if type(hh) == 'table' then
	return metric(function(x,y,i) return korn.gaussian(x-y,hh[i],ws[i]) end, function(x) return #x end, true)
    else
        return metric(function(x,y) return korn.gaussian(x-y,hh,ws) end, function(x) return #x end, true)
    end
end
--]]

