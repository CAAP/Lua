--module setup
local M = {}

-- Import section
local cv = require'locv'
local gdcm = require'lgdcm'
local math = math
local table = table
local string = string
local pairs = pairs
local ipairs = ipairs
local print = print
local assert = assert
local tonumber = tonumber

local sql = require'carlos.sqlite'
local dcm = require'carlos.dicom'
local fs = require'carlos.files'
local tb = require'carlos.tables'
local st = require'carlos.string'

local neu = require'carlos.neuro'
local ios = require'carlos.io'
local ssl = require'carlos.openssl'

local tr = require'carlos.transducers'
local fd = require'carlos.fold'

local ops = require'carlos.operations'
local seg = require'carlos.segment'
local ex = require'carlos.exponentials'
local his = require'carlos.histogram'
local sts = require'carlos.stats'

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

local tags = {dcm.tags.itype, dcm.tags.date, dcm.tags.mdty, dcm.tags.stdy, dcm.tags.desc, dcm.tags.pid, dcm.tags.thick, dcm.tags.space, dcm.tags.prot, dcm.tags.ppos, dcm.tags.oid, dcm.tags.uid, dcm.tags.srs, dcm.tags.num, dcm.tags.ipp, dcm.tags.iop, dcm.tags.rows, dcm.tags.cols, dcm.tags.resol }

table.sort( tags, function(x,y) return x[1] < y[1] or ( x[1] == y[1] and x[2] < y[2]) end )

local gridw = seg.gridw
local gridh = seg.gridh
local sub = seg.sub

-- Local functions for module-only access

---------------------------------
-- Public function definitions --
---------------------------------
function M.readProfile(dbconn, root)
    local ptags = dcm.tags.confidential
    dbconn.exec'CREATE TABLE IF NOT EXISTS paths (path)'
    neu.makeTable( dbconn, 'profile', ptags )

    fd.slice( 100, ios.lines( string.format('find -L %q -type f', root) ), fd.map( function(p) return {p} end ), sql.into'paths', dbconn )

    print'\n'

    local N = dbconn.count'paths'
    fd.slice( 100, dbconn.query'SELECT path from paths' , neu.validDCM( ), neu.readDCM( ptags ), st.status(N), sql.into'profile', dbconn )

    print'\n'
end

function M.readFiles(dbconn, root)
    dbconn.exec'CREATE TABLE IF NOT EXISTS paths (path)'
    neu.makeTable( dbconn, 'raw', tags )

    fd.slice( 100, ios.lines( string.format('find -L %q -type f', root) ), fd.map( function(p) return {p} end ), sql.into'paths', dbconn )

    print''

    local N = dbconn.count'paths'
    fd.reduce( dbconn.query'SELECT path from paths' , neu.validDCM(), neu.readDCM(tags), st.status(N), sql.into'raw', dbconn )

    print'\n'
end

function M.sorting( dbconn, orientation )
    dbconn.exec'CREATE TABLE IF NOT EXISTS coordinates (path primary key, UID, dx NUMBER, dy NUMBER, z NUMBER)'
    local QRY = 'SELECT path, UID, IOP, IPP, resolution FROM raw WHERE IOP NOT NULL'
    local z = fd.map( function(img) dcm.z(img); dcm.resolution(img); return img end )
    local oriented = fd.filter( function(img) return dcm.orientation(img) == orientation end )
    local N = dbconn.count('raw', 'WHERE IOP NOT NULL')

    fd.slice( 100, dbconn.query( QRY ), oriented, z, neu.getValues( dbconn, 'coordinates' ), st.status(N), sql.into'coordinates', dbconn )
    print''

    dbconn.exec'CREATE TABLE IF NOT EXISTS spacing (UID PRIMARY KEY, zmin NUMBER, count NUMBER, dz NUMBER)'
    dbconn.exec'INSERT INTO spacing SELECT UID, min(z), COUNT(path), ROUND((max(z)-min(z))/(COUNT(path)-1),4) FROM coordinates GROUP BY UID'

    dbconn.exec(string.format( 'CREATE TABLE IF NOT EXISTS %q (path PRIMARY KEY, UID, PID, date, rows NUMBER, cols NUMBER, image_number NUMBER, dx NUMBER, dy NUMBER, dz NUMBER, slice NUMBER, grid NUMBER)', orientation ))
    dbconn.exec(string.format( 'INSERT INTO %q SELECT raw.path, raw.UID, PID, study_date, rows, cols, ROUND((z-zmin)/dz), dx, dy, dz, -1, -1 FROM coordinates, spacing, raw WHERE coordinates.UID = spacing.UID AND coordinates.path = raw.path', orientation ))
    dbconn.exec(string.format( 'UPDATE %q SET slice = ROUND(image_number / %d) + 1, grid = image_number %% %d + 1', orientation, gridw*gridh, gridw*gridh ))
    print''
end

function M.tiles(dbconn, parent, orientation)
    local PIDs = string.format( 'SELECT DISTINCT PID FROM %q', orientation )

    fd.reduce( dbconn.query(PIDs), function(p) neu.mkdirs( parent, {'PID'}, p ) end )

-- --- --- --

    dbconn.exec'CREATE TABLE IF NOT EXISTS tiles ( path, UID, PID, rows INTEGER, cols INTEGER, dx NUMBER, dy NUMBER, dz NUMBER, slice INTEGER, count INTEGER)'
    dbconn.exec( string.format("INSERT INTO tiles SELECT '', UID, PID, rows, cols, dx, dy, dz, slice, count(grid) FROM %q GROUP BY UID, slice", orientation) )

-- --- --- --

    local images = string.format( 'SELECT * FROM %q ORDER BY UID, slice, grid DESC', orientation )

    local function updatePath(w)
	w.path = string.format('%s/%s_%d_$TYPE.png', neu.getPath( parent, {'PID'}, w[1] ), ssl.md5(w[1].UID):sub(1,6), w[1].slice )
	return w
    end

    local function makeTiles(w)
	seg.tile( w )
	return {w.path, w[1].UID, w[1].slice}
    end

    local N = dbconn.count( orientation )

    fd.slice( 100, dbconn.query(images), fd.sliceBy( function(image) return image.grid == 1 end ), fd.map( updatePath ), fd.map( makeTiles ), st.status(N), sql.update{ 'tiles', 'path', {'UID', 'slice'} }, dbconn )
end

function M.braindamage( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS surgery (path PRIMARY KEY)'

    local tiles = 'SELECT * FROM tiles ORDER BY path DESC'

    local N = dbconn.count'tiles'

    fd.reduce( dbconn.query(tiles), seg.surgery, st.status(N), sql.into'surgery', dbconn  )
end

function M.histogram( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS histogram (path PRIMARY KEY, UID, slice INTEGER, histogram)'

    local getHistogram = fd.map( function(tile) return seg.histogram(tile) end )

    local getValues = fd.map( function(tile) return {tile.path, tile.UID, tile.slice, table.concat(tile.histogram,';') } end )

    local tiles = 'SELECT * FROM tiles WHERE path IN (SELECT path FROM surgery)'

    local N = dbconn.count'surgery'

    fd.reduce( dbconn.query(tiles), getHistogram, getValues, st.status(N), sql.into'histogram', dbconn )
end

function M.binomial( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS binomial (UID PRIMARY KEY, N integer, w1 DOUBLE, w2 DOUBLE, eta1 DOUBLE, eta2 DOUBLE, pixelCount INTEGER, mni INTEGER, mxi INTEGER, bayes INTEGER, lut)'

    local function aggregate(h, tile) fd.reduce(tile.histogram, function(x,i) h[i] = h[i] and h[i] + x or x end) end

    local function accumulate(step) local h = {}; return function( tile, i ) aggregate(h, tile); if tile.slice == 1 then tile.histogram = h; step(tile, i); h = {} end end end

    local getHistogram = fd.map( function(tile) tile.histogram = st.split(tile.histogram, ';'); return tile end )

    fd.reduce( dbconn.query'SELECT * FROM histogram ORDER BY path DESC', getHistogram, accumulate, fd.map( seg.MoB ), sql.into'binomial', dbconn )
end

function M.threshold( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS threshold (path PRIMARY KEY, pixelCount INTEGER)'

    local tiles = 'SELECT path, histogram.UID, slice, mni, mxi FROM histogram, binomial WHERE histogram.UID == binomial.UID'

    local N = dbconn.count'histogram'

    fd.reduce( dbconn.query( tiles ), fd.map( seg.threshold ), st.status(N), sql.into'threshold', dbconn )
end

function M.brain( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS stripping (path PRIMARY KEY)'

    local tiles = 'SELECT * FROM tiles WHERE path IN (SELECT path FROM surgery) ORDER BY path DESC'

    local N = dbconn.count'surgery'

    fd.reduce( dbconn.query( tiles ), seg.stripping, st.status(N), sql.into'stripping', dbconn )
end

function M.beam( dbconn )
    local tiles = 'SELECT path FROM tiles WHERE path IN (SELECT path FROM stripping)'

    local N = dbconn.count'stripping'

    fd.reduce( dbconn.query( tiles ), fd.comp{st.status(N), seg.artifact} )
end

function M.icv( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS icv (UID PRIMARY KEY, PID, pixelCount INTEGER, volmm3 NUMBER, histogram)'

    local tiles = 'SELECT * FROM tiles WHERE path IN (SELECT path FROM stripping) ORDER BY path DESC'

    local N = dbconn.count'stripping'

    fd.reduce( dbconn.query( tiles ), seg.icv, st.status(N), sql.into'icv', dbconn )
end

function M.bayes( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS bayes (UID PRIMARY KEY, PID, bayes INTEGER, icvVol NUMBER, brainVol NUMBER, csfVol NUMBER)'

    local tiles = 'SELECT * FROM binomial, icv WHERE binomial.UID == icv.UID'

    local N = dbconn.count'stripping'

    fd.reduce( dbconn.query( tiles ), seg.bayes, st.status(N), sql.into'bayes', dbconn )
end

function M.csfHistogram( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS csf (UID PRIMARY KEY, PID, histogram)'

    local tiles = 'SELECT * FROM tiles, icv, binomial WHERE tiles.UID == icv.UID AND tiles.UID == binomial.UID AND path IN (SELECT path FROM stripping) ORDER BY path DESC'

    local N = dbconn.count'stripping'

    fd.reduce( dbconn.query( tiles ), seg.csfHistogram, st.status(N), sql.into'csf', dbconn )
end

function M.volumes( dbconn )
    dbconn.exec'CREATE TABLE IF NOT EXISTS volumes (UID PRIMARY KEY, PID, bayes INTEGER, histogram, icvVol NUMBER, brainVol NUMBER, csfVol NUMBER)'

    local tiles = 'SELECT * FROM tiles, icv, binomial WHERE tiles.UID == icv.UID AND tiles.UID == binomial.UID AND path IN (SELECT path FROM stripping) ORDER BY path DESC'

    local N = dbconn.count'stripping'

    fd.reduce( dbconn.query( tiles ), seg.csfRegion, st.status(N), sql.into'volumes', dbconn )
end

local function agreement( dbconn, tbname, query )
    dbconn.exec( 'CREATE TABLE IF NOT EXISTS ' .. tbname .. ' (PID PRIMARY KEY, average NUMBER, difference NUMBER)' )

    local intoPairs = fd.sliceBy( function(_,j) return j % 2 == 0 end ) -- proper use of sliceBy

    local function blandAltman( pair )
	local vol1, vol2 = pair[1].volume, pair[2].volume
	local avg = (vol1+vol2)/2
	local dif = vol1-vol2
	return { pair[1].PID, avg, dif }
    end

    fd.reduce( dbconn.query( query ), intoPairs, fd.map( function(w) return blandAltman(w) end ), sql.into( tbname ), dbconn )

    -- statistics on the differences --
---[[
    local stats = fd.reduce( dbconn.query('SELECT difference FROM '.. tbname),fd.map( function(x) return x.difference end ), fd.stats, {} )
    stats.s2 = stats.ssq/(stats.n-1)
    local sd = math.sqrt( stats.s2 )
    local ttest = sts.ttest( sts.one( stats ) )
    dbconn.exec(string.format('INSERT INTO %q VALUES ( %q, %f, %f)', tbname, 'BIAS', stats.avg, ttest.p))
    dbconn.exec(string.format('INSERT INTO %q VALUES ( %q, %f, %f)', tbname, 'STDEV', sd, 2*sd))
--]]
end

function M.icvAgree( dbconn )
    local icvsQuery = 'SELECT PID, round(icvVol/1e3) volume FROM bayes ORDER BY PID'
    agreement( dbconn, 'icvAgree', icvsQuery )
end

function M.brainAgree( dbconn )
    local brainQuery = 'SELECT PID, round(brainVol/1e3) volume FROM bayes ORDER BY PID'
    agreement( dbconn, 'brainAgree', brainQuery )
end

function M.csfAgree( dbconn )
    local csfQuery = 'SELECT PID, round(csfVol/1e3) volume FROM bayes ORDER BY PID'
    agreement( dbconn, 'csfAgree', csfQuery )
end

function M.ratioAgree( dbconn )
    local qry = 'SELECT PID, round(brainVol/csfVol,1) volume FROM bayes ORDER BY PID'
    agreement( dbconn, 'ratioAgree', qry )
end

local function overlay( tile, mask, suffix )
    local msk = cv.open( sub( tile, mask ) ):range(1, 255)
    local skull = cv.open( sub( tile, 'A' ) ):overlay( msk )
    skull:save( sub( tile, suffix ))
end

function M.icvOverlay( dbconn )
    fd.reduce( dbconn.query'SELECT path from stripping', function(tile) overlay(tile, 'Z', 'tICV') end )
end

function M.csfOverlay( dbconn )
    local function draw( tile )
        local icv = cv.open( sub( tile, 'Z' ) ):range(1, 255)
	local csf 
        local skull = cv.open( sub( tile, 'A' ) ):overlay( icv )
	skull:save( sub( tile, csf ))
    end

    fd.reduce( dbconn.query'SELECT path from stripping', draw )
end

return M
