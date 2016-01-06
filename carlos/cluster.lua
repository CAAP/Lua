-- module setup
local M = {}

-- Import Section
local ap = require'lapc'
local fd = require'carlos.fold'
local mt = require'carlos.metric'

local table = table
local print = print
local math = math
local error = error

-- Local Variables for module-only access
local lambda = 0.8
local convits = 100 -- convergence iterations
local maxits = 1000 -- maximum number of iterations

-- No more external access after this point
_ENV = nil -- or M

--------------------------------
-- Local function definitions --
--------------------------------
    -- SIMILARITIES has a structure like:
    -- [i, j, s]
    -- indices i,j -> 1,2
    -- similarity -> 3

local function apcluster(matrix, options)
    local opts = options or {lambda=lambda, convits=convits, maxits=maxits}
    local idx, netsim, iter = ap.cluster(matrix, opts)

    if iter > 0 then
	print('APCluster reached convergence after: '.. iter ..' iterations.')
	print('Net similarity: '.. netsim)

	local uqs = fd.reduce( idx, fd.unique(), fd.into, {} ) -- exemplars
	print('Affinity propagation found: '.. #uqs ..' clusters.')

	local MM = {indices=idx, netsim=netsim, exems=uqs}
	function MM.cluster( points, fsim )
	    local index = fd.map( function(y) return fd.reduce( uqs, fd.map( function(x) return fsim(x, y) end ), fd.max, {} ).idx end )
	    return fd.reduce( points, index, fd.into, {} )
	end
        return MM
    end

    error('Exit with error ('.. iter .. ') in APCLUSTER.')
end

local function byindex(x, y) return (x[1] < y[1]) or (x[1] == y[1] and x[2] < y[2]) end

local function bysimilarity(x, y) return x[3] < y[3] end

local function median( data )
    local N = #data
    local k = N/2
    return (N%2 == 1) and data[(N+1)/2][3] or (data[k][3] + data[k+1][3])/2
end

---------------------------------
-- Public function definitions --
---------------------------------

M.apcluster = apcluster

function M.affinity(similarities, damping)
    local sims = similarities

    -- based on 3rd element
    table.sort( sims, bysimilarity )
    local pref = median( sims )

    lambda = damping or lambda

    -- initialize preference -- run over i's | 1st element
    fd.reduce( sims, fd.map( function(s) return s[1] end ), fd.unique(), fd.map( function(i) return {i, i, pref} end ) , fd.into, sims )
   
    table.sort(sims, byindex) -- APCLUSTER requires a proper order of indices

    if sims[1][1] == 1 then fd.reduce( sims, function(s) s[1] = s[1] - 1; s[2] = s[2] - 1 end ) end

--    return apcluster( sims )
    return sims
end

return M

