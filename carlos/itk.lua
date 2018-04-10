-- module setup
local M = {}

-- Import Section
local assert = assert
local ipairs = ipairs
local pairs = pairs
local print = print
local string = string
local table = table

local fd = require'carlos.fold'
local itk = require'litk'

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access


--------------------------------
-- Local function definitions --
--------------------------------

local function maxBounds( a )
    a.max = a.bounds
    a.idx = 1
    return function( v, i )
	fd.reduce(v.bounds, function(x,j) if x > a.max[j] then a.max[j] = x; a.idx = i end end)
    end
end

local function byBounds(a, b)
    local u = a.bounds
    local v = b.bounds
    u[0] = u[1]
    v[1] = v[1]
    return u[1] > v[1] or fd.any(u, function(x,i) return u[i-1] == v[i-1] and x > v[i] end)
end

local function eqBounds(u, v) return fd.any(u, function(x,i) return x ~= u[i] end) end

---------------------------------
-- Public function definitions --
---------------------------------

function M.dcmSeries( data, path, f )
print('\nConverting series', data[1].series_desc)
print('Transforming',  #data ,'files.\n')
    local info = itk.info(data[1].path)
    local fs = fd.reduce(data, fd.map(function(x) return x.path end), fd.into, {})
print('Trying pixel type ', info.type, '\n')
    if f then f(path) end
    return itk.dcmSeries(info.ctype, fs, path)
end

function M.average(paths, outpath, normp)
print('Averaging', #paths, 'files.\n')
    local infos = fd.reduce(paths, fd.map(itk.info), fd.into, {})
    table.sort(infos, byBounds) -- index of image with maximum bounds is 1
    local mx = infos[1].bounds
print('Bounds are (', table.concat(mx, ', '), ').\n')
    local _,rix = fd.first(infos, function(a) return eqBounds(mx, a.bounds) end) -- resampling index
    if rix then print('There are', #paths - rix + 1, 'elements with different bounds!\n') end
print('Averaging image, output will be written to', outpath, '\n')
    return itk.average(paths, 1, normp or false, outpath, rix or  0)
end


function M.normalize(fixed, moving)
print('Running ANTS-like normalization script.\n')

print('Transformation used is SyN[1.0].\n')
print('Regularization used is Gauss[3, 0.5].\n')

    local N = l
    local levels = math.floor(math.log(N/32, 2)) + 1

print('Iterations set to', iter, '\n')
end


return M

