-- module setup
local M = {}

-- Import Section
local fd = require'carlos.fold'

local maxi=math.max
local mini=math.min
local mod=math.fmod
local print=print

local concat = table.concat

-- Local Variables for module-only access
local huge=math.huge
local lambda= 0.8
local ilambda = 1 - lambda
local convits=200 -- convergence iterations
local maxits=2000

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

    ----------------------------------------------
    -- POINTS has a structure like:
    -- p -> [i, j, s, a, r, ie; count = #]
    -- indices i,j -> 1,2
    -- similarity -> 3
    -- availability -> 4
    -- responsability -> 5
    ----------------------------------------------
    -- similarity indicates how well data point k is suited to be exemplar of point i
    -- availability sent from center k to i's for how appropriate for i is o choose k as exemplar
    -- responsability exemplar k receives from i's on how suitable point k is to be exemplar for point i
    ----------------------------------------------
    -- exemplars FN() when called produces a label
    -- isExemplar (true) -> 6 
    ----------------------------------------------
    -- i : index over the points
    -- j : index of another point
    -- count : # unique points
    ----------------------------------------------

-- Private function definitions --

local function max1(mxs)
    return function( p )
	local i, ra  = p[1], p[4]+p[5]
	if ra > mxs[i][1] then mxs[i] = {ra, p[2]} end
    end
end

local function maxx(mxs)
    return function( p )
	if p[1] == p[2] then return end -- should not send msg to itself
	local i = p[1]
	local sa, mx = p[3]+p[4], mxs[i]
	if sa > mx[2] then mx[2] = sa; if sa > mx[1] then mxs[i] = {sa, mx[1], p[2]} end end
    end
end

local function updater(mxs)
    return function(p)
	local mx = mxs[p[1]]
	local rho = p[3] - (mx[3] == p[2] and mx[2] or mx[1])
	p[5] = p[5]*lambda + ilambda*rho
    end
end -- rho = s - max_(s+a)

local function isexemplar(p) return (p[4]+p[5]) > 0 end

local function count(a) return function() a.count = a.count + 1 end end

local function padding(cnts,index)
    if cnts[index] == 0 then return convits end
    for i=1,convits-1 do
	local k = mod(index+i-1, convits) + 1
	if not(cnts[k]==cnts[index]) then return i-1  end
    end
    return 0
end

local function responsability( points, diagonal )
    -- initialization
    local max_sa = fd.reduce( diagonal, function(mxs) return function(p) mxs[p[1]] = {-huge, -huge, -1} end end, {} )
    -- find exemplar k that maximize s+a
    fd.reduce( points, maxx(max_sa) )
    --update responsability
    fd.reduce( points, updater(max_sa) )
end

local function availability( offaxis, diagonal )
    -- initialization
    local sum_pos = fd.reduce( diagonal, function(sms) return function(p) sms[p[2]] = 0 end end, {} )
    -- sum of positive responsabilities; except self-responsabilities
    fd.reduce( offaxis, function(sms) return function(p) if p[5] > 0 then sms[p[2]] = sms[p[2]] + p[5] end end end, sum_pos );
    -- update self-availability
    fd.reduce( diagonal, function(p) p[4] = p[4]*lambda + ilambda*sum_pos[p[2]] end )
    -- add self-responsability
    fd.reduce( diagonal, function(p) sum_pos[p[2]] = sum_pos[p[2]] + p[5] end )
    -- update availability
    fd.reduce( offaxis, function(p) local s = mini(0, sum_pos[p[2]] - maxi(p[5], 0)); p[4] = p[4]*lambda + ilambda*s end )
end

local function criterion( points, diagonal )
    -- initialization
    local max_ra = fd.reduce( diagonal, function(mx) return function(p) mx[p[1]] = {-huge, -1} end end, {} )
    -- find exemplar k that maximize r+a
    fd.reduce( points, max1(max_ra) )

    max_ra = fd.reduce( max_ra, fd.map(function(v) return v[2] end), fd.into, {} )
    print( concat(max_ra, '\t') )
end

local function init(points)
    if not points.diagonal then points.diagonal = fd.reduce( points, fd.filter( function(x) return x[1] == x[2] end), fd.into, {} ) end
    if not points.offaxis then points.offaxis = fd.reduce( points, fd.filter( function(x) return x[1] ~= x[2] end), fd.into, {} ) end
end

---------------------------------
-- Public function definitions --
---------------------------------

M.responsability = responsability
M.availability = availability

function M.count( points )
    init(points)
    return fd.reduce( points.diagonal, fd.filter(isexemplar), count, {count=0} ).count
end

function M.step( points )
    init(points)
    responsability( points, points.diagonal )
    availability( points.offaxis, points.diagonal )
    criterion( points, points.diagonal )
    return true
end

function M.assign( points )
    init(points)
    local diagonal, offaxis = points.diagonal, points.offaxis
    -- identify exemplars
    local exemps = fd.reduce( diagonal, fd.filter(isexemplar), fd.rejig( function(p) return true, p[2] end ), fd.merge, {} )
    -- initialization
    fd.reduce( diagonal, function(p) if exemps[p[1]] then p.max = p[3]; p.idx = p[2] else p.max = -huge; p.idx = -1 end end )
    -- filter data to maximize
    local nonexemps = fd.reduce( offaxis, fd.filter( function(p) return not(exemps[p[1]]) and exemps[p[2]] end ), fd.into, {} )
    -- maximum similarity
    fd.reduce( nonexemps, function(p) local q = diagonal[p[1]]; if p[3] > q.max then q.max = p[3]; q.idx = p[2] end end )
end

function M.converge( points )
    init(points)
    local counts, pad, index, diagonal, offaxis = {}, convits, -1, points.diagonal, points.offaxis

    for k=1, maxits do
	index = mod(k-1, convits) + 1
	if k>pad then
	    pad = pad + padding( counts, index )
	    if k>pad then return k, counts[index] end
	end
	responsability(points, diagonal); availability(offaxis, diagonal)
	counts[index] = fd.reduce( diagonal, fd.filter(isexemplar), count, {count=0} ).count
    end
end

return M

