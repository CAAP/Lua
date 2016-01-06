--module setup
local M = {}

-- Import section
local cv = require'locv'
local gdcm = require'lgdcm'
local math = math
local ipairs = ipairs
local print = print
local assert = assert
local tonumber = tonumber
local concat = table.concat

local fd = require'carlos.fold'

local tr = require'carlos.transducers'
local ops = require'carlos.operations'
local his = require'carlos.histogram'
local ex = require'carlos.exponentials'
local str = require'carlos.string'

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

local gridw, gridh = 4, 5
local hmin, hmax, bins, step = 20, 145, 126, 1
local thre = 0.005 -- robust statistics
local K = 120 -- binomial

-- Local functions for module-only access
local function round(x, w)
    return math.floor( x * w + 0.5 )
end

local function zeros( p )
    local ans = {}; for j=1,p do ans[j] = 0 end
    return ans
end

local function tile(images)
    local path, rows, cols = images.path, images[1].rows, images[1].cols
    local r = cv.rect{x=0, y=0, width=cols*gridw, height=rows*gridh}
    local grid = r:grid( cols, rows )
    local tissue = r:zeros()
    local bone = r:zeros()
    local head = r:zeros()

    for _,img in ipairs(images) do
        local a = gdcm.open(img.path)
        if not(a) then print( string.format('Could not open image file: slice %d, index %d\n\t%q\n', img.slice, img.grid, img.path), 'Skipping image.\n')
        else
	    local b = cv.openDCM(a)
	    local _,mx = b:minmax()
	    local b1 = b:range(200, mx) -- BONE
	    if (mx > 200 and b1:nonzero() > 1e3) then
		local c = b1:clone():contours'EXT'
		c = tr.filter( c, function(x) return x:property'AREA' > 1e3 end)
		if #c > 0 then
		    local mask = b:range(-50, 205) -- TISSUE
		    if mask:nonzero() > 1e3 then
			local roi = grid[img.grid]
			-- BONE --
			b1 = b1:copy(b1:draw(c))
			b1:assign( roi:apply(bone) )
			-- TISSUE --
			local b2 = b:add(50, mask):copy(mask):tobyte(mask)
			if b:minmax() == 0 then b2 = b2:add(50, mask) end
			b2:assign( roi:apply(tissue) )
			-- HEAD = Bone + Tissue --
			b1:assign( b2, b1 )
			b2:assign( roi:apply(head) )
		    else print('Regions in tissue-mask are too small', img.path) end
	        else print('No valid areas in bone-mask', img.path) end
	    else print('No bone-mask found', img.path) end
	end
    end

    tissue:save( path:gsub('%$TYPE', 'T') )
    bone:save( path:gsub('%$TYPE', 'B') )
    head:save( path:gsub('%$TYPE', 'A') )

end

local function sub(tile, sufix)
    return tile.path:gsub('%$TYPE', sufix)
end

local function getGrid(rows, cols)
    return cv.rect( {x=0, y=0, width=cols*gridw, height=rows*gridh} ):grid( cols, rows )
end

local function neurosurgery(previous, tile)
    local grid = getGrid( tile.rows, tile.cols )
    local bone = cv.open( sub(tile, 'B') )
    if bone:nonzero() < 1e3 then return nil end
    local tissue = cv.open( sub(tile, 'T') )

    local c = bone:clone():contours'EXT'
    local b1 = bone:draw(c)
    local b2 = b1:bitwise(bone, 'XOR') -- interior of skull
    if b2:nonzero() < 1e3 then return nil end
    local ret = bone:zeros()
    
    local function init()
	local k = tile.count
	while k > 0 do if grid[k]:apply( b2 ):nonzero() > 500 then return k end; k = k - 1 end
	return 0
    end

    local k = previous and tile.count or init()
    if k == 0 then return nil end
    local area = previous and previous:nonzero() or grid[k]:apply( b2 ):nonzero() --1e3
    local flag = true
    -- process each grid cell => k
    while k > 0 do
	local roi = grid[k] -- roi := cell
	local b3 = roi:apply(b2):clone() -- interior of skull
	flag = flag and b3:nonzero() / area > 0.8
	if flag then
	    -- take tissue inside the skull
	    b3 = roi:apply( tissue ):clone():copy( b3 )
        else
            -- if area is decreasing (compared to previous cell), take tissue and limit area to previous (largest) one
            b3 = roi:apply( tissue ):clone()
            b3 = grid[k+1] and b3:copy( grid[k+1]:apply( ret ):range(1, 255) ) or b3:copy( previous:range(1, 255) )
	end
	-- save results in ret
	b3:assign( roi:apply( ret ) )
	area = b3:nonzero()
	k = k -1
    end
    -- save result
    ret:save( sub(tile, 'V') )
    return grid[1]:apply( ret ):clone()
end


local function idle(alef, beta)
    return function(previous, tile)
	local grid = getGrid( tile.rows, tile.cols )
	local b1 = cv.open( sub(tile, alef) ):range(1, 255):morphology('OPEN', 'ELLI', 3)
	if b1:nonzero() < 1e3 then return nil end
	local ret = b1:zeros()

	local function init()
	    local k = tile.count
	    while k > 0 do if grid[k]:apply( b1 ):nonzero() > 500 then return k end; k = k - 1 end
	    return 0
	end

	local k = init()
	if k == 0 then return nil end
	local mask = previous and previous or grid[k]:apply( b1 ) -- first grid

    -- process each grid cell => k
	while k > 0 do
	    local roi = grid[k] -- current grid
	    local b2 = roi:apply( b1 ):clone() -- interior of skull
	    if b2:nonzero() < 500 then k = 0
	    else
		local c1 = b2:clone():contours'EXT'
		local overlap = mask:bitwise( b2, 'AND' )
        	c1 = tr.filter( c1, function(x) return overlap:copy( b2:draw{ x } ):nonzero() > 50 end )
		mask = b2:draw( c1 )
		mask:assign( roi:apply( ret ) )
		k = k - 1
            end
	end

    -- save result
	local brain = cv.open( sub(tile, alef) ):copy( ret )
	brain:save( sub(tile, beta) )
	return grid[1]:apply( ret ):clone(), ret:nonzero(), brain:histogram(0, 256, 256)
    end
end

local function cycle(f) return function(step) local ret = nil; return function(tile, i) ret = f(ret, tile); if ret then step({tile.path}, i) end; if tile.slice == 1 then ret = nil end end end end

---------------------------------
-- Public function definitions --
---------------------------------

M.gridw = gridw
M.gridh = gridh

M.tile = tile
M.sub = sub

M.surgery = cycle(neurosurgery)

function M.histogram( tile )
    tile.histogram = cv.open( sub(tile, 'V') ):histogram( hmin, hmax+1, bins )
    return tile
end

local keys = ops.range( hmin, hmax, step )

local function params( bino, mm )
    local eta1, eta2 = mm.etas[1], mm.etas[2]
    local lambda1 = bino.eta2lambda( eta1 )
    local lambda2 = bino.eta2lambda( eta2 )
    local it = fd.first( ops.range(math.floor(eta1), math.floor(eta2)), function(k) return bino.density(k, lambda1) < bino.density(k, lambda2) end )
    return it, eta1, eta2
end

function M.MoB( aggregate )
    local h = aggregate.histogram
    local ks, cts = his.robust( keys, h, thre )
    local bino = ex.binomial( K )
    local mm = ex.mixtureModel( bino, {50, 80}, {1/4, 3/4} )

    ex.histoSoft(ks, cts, mm)

    assert( params( bino, mm ), 'ERROR: EM step failed.\n'..aggregate.path )

    local N = fd.reduce( h, fd.sum, {} ).sum
    local lut = fd.times( 256, fd.map(function(j) return round( mm.density(j), N ) end), fd.into, {} )
    local pxcount = fd.reduce( keys, fd.filter( function(k) return lut[k] > 0 end ), fd.map( function(k) return h[k] end ), fd.sum, {} ).sum -- CAN BE SKIPPED
    local mni = fd.first( keys, function(k) return lut[k] > 0 end )
    local mxi = fd.first( keys, function(k) return k > mni and lut[k] == 0 end )
    local it, eta1, eta2 = params( bino, mm )

    return { aggregate.UID, K, mm.ws[1], mm.ws[2], eta1, eta2, pxcount, mni, mxi, it, concat(lut, ';') }
end

function M.threshold( tile )
    local img = cv.open( sub(tile, 'V') )
    local msk = img:range( tile.mni, tile.mxi )
    img:copy(msk):save( sub(tile, 'W') )
    return { tile.path, img:nonzero() }
end

M.stripping = cycle(idle('W', 'Y'))

-- correct for beam-hardening artifact
function M.artifact(tile)
    local v = cv.open( sub(tile, 'V' ) )
    local y = cv.open( sub(tile, 'Y' ) ):range(1, 255)
    local xor = y:bitwise( v:range(1,255), 'XOR' )
    local overlap = y:morphology( 'DILATE', 'ELLI', 5):bitwise( xor, 'AND' ) -- dilate to be any overlap at all
    local cs = xor:clone():contours'EXT'
    cs = tr.filter( cs, function(x) return x:roi():apply( overlap ):nonzero() > 50 end ) --fairly OK!!! roi() fn disregards neighboring regions
    local ret = y:bitwise( xor:draw(cs), 'OR' )
    v:copy( ret ):save( sub(tile, 'Y1') )
end

function M.icv(step)
    local count, pxcnt, ret, h, hh = 0, 0, nil, zeros(256), nil
    local _f = idle( 'Y1', 'Z' )
    return function(tile, i)
	ret, pxcnt, hh = _f(ret, tile) 
	count = count + pxcnt
	fd.reduce( hh, function(q,j) h[j] = h[j] + q end )
	if tile.slice == 1 then step({tile.UID, tile.PID, count, count * tile.dx * tile.dy * tile.dz, concat(h,';')}, i); count = 0; ret = nil; h = zeros(256); end
    end
end

---[[
function M.bayes( step )
    return function( series, i )
	local eta1, eta2 = series.eta1, series.eta2
	local bino = ex.binomial( series.N )
	local lambda1 = bino.eta2lambda( eta1 )
	local lambda2 = bino.eta2lambda( eta2 )
	local it = fd.first( ops.range(math.floor(eta1), math.floor(eta2)), function(k) return bino.density(k, lambda1) < bino.density(k, lambda2) end )
	local h = str.split( series.histogram, ';' )
	h[1] = 0 -- background
	local csf = fd.take( it, h, fd.sum, {} ).sum
	local icv = fd.reduce( h, fd.sum, {} ).sum
	local brain = icv - csf
	local dv = series.volmm3/series.pixelCount
	step({ series.UID, series.PID, it, icv*dv, brain*dv, csf*dv }, i)
    end
end
--]]

local function greatest(tile, mxcount)
    local grid = getGrid( tile.rows, tile.cols )
    local brain = cv.open( sub(tile, 'Z') )
    local b1 = brain:range(hmin, tile.bayes):morphology('ERODE', 'ELLI', 3)
--    local b2 = brain:range( hmin, hmax)
--    local ratio = fd.filter(function(roi) local q = roi:apply( b1 ):nonzero()/roi:apply( b2 ):nonzero(); return q<0.6 and q>0.2 end)
    local big = fd.take( tile.count, grid, fd.map(function(roi) return roi:apply( b1 ):nonzero() end), fd.max, {max=mxcount} )
    return big.idx and grid[big.idx]:apply( brain ):histogram(0, 256, 256), big.max
end
---[[
function M.csfHistogram( step )
    local h, hh, pxcnt = nil, nil, 0
    return function(tile, i)
	hh, pxcnt = greatest( tile, pxcnt )
	if hh then h = hh end
	if tile.slice == 1 then
	    step({ tile.UID, tile.PID, concat(h, ';') }, i)
	    h = zeros(256); pxcnt = 0
	end
    end
end

function M.csfRegion( step )
    local h, hh, pxcnt = nil, nil, 0
    return function(tile, i)
	hh, pxcnt = greatest( tile, pxcnt )
	if hh then h = hh end
	if tile.slice == 1 then
	    local cts = fd.reduce( keys, fd.map( function(k) return h[k] end ), fd.into, {} )
	    local ks, cts = his.robust( keys, cts, thre )
	    local bino = ex.binomial( K )
	    local mm = ex.mixtureModel( bino, {50, 80}, {0.3, 0.7} )
	    ex.histoSoft( ks, cts, mm )
	    local it, eta1, eta2 = params( bino, mm )
	    assert( it , 'ERROR: estimated value cannot be nil:\t' .. i .. tile.path)
	    local hall = str.split(tile.histogram, ';')
	    hall[1] = 0 -- background
	    local csf = fd.take( it, hall, fd.sum, {} ).sum
	    local icv = fd.reduce( hall, fd.sum, {} ).sum
	    local brain = icv - csf
	    local dv = tile.dx * tile.dy * tile.dz
	    step({ tile.UID, tile.PID, it, concat(h, ';'), icv*dv, brain*dv, csf*dv }, i)
	    h = zeros(256); pxcnt = 0
	end
    end
end

return M

