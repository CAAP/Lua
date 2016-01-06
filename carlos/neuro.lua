--module setup
local M = {}

-- Import section
local sql = require'carlos.sqlite'
local dcm = require'carlos.dicom'
local fd = require'carlos.fold'

local table = table
local string = string
local io = io
local os = os
local assert = assert
local print = print
local tonumber = tonumber

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

-- Local functions for module-only access

local function splitpath( s )
    local j = 0
    repeat j = s:find('/', j+1) until not s:find('/', j+1)
    return s:sub(1,j-1), s:sub(j+1,-1)
end

---------------------------------
-- Public function definitions --
---------------------------------
M.SERIES = 'CREATE TABLE IF NOT EXISTS series (UID primary key, PID, modality, orientation, series_desc, date, IOP, series_number NUMBER, count NUMBER, BO NUMBER, TE NUMBER, TR NUMBER, rows NUMBER, cols NUMBER, dx NUMBER, dy NUMBER)'

M.SRSQ = 'SELECT *, study_date date, COUNT(*) count FROM raw WHERE IOP NOT NULL GROUP BY UID'

M.IOPQ = 'SELECT * FROM raw WHERE IOP NOT NULL'

M.splitpath = splitpath

function M.dirpath( s )
    local dir = splitpath( s )
    return dir
end

function M.countUID( dbconn )
    local qry = 'SELECT COUNT(*) cnt FROM (SELECT UID from raw WHERE IOP NOT NULL GROUP BY UID)'
    return fd.first( dbconn.query( qry ), function(x) return x.cnt end ).cnt
end

function M.validDCM( tags )
    local reader = dcm.reader( tags or dcm.tags.some )
    return fd.filter( function(f) return reader:valid(f.path) end )
end

function M.readDCM( tags )
    local reader = dcm.reader( tags or dcm.tags.all )
    return fd.map( function(f) return reader:read(f.path) end )
end

function M.readProfile( )
    local reader = dcm.confidential()
    return fd.map( function(f) return reader:read(f.path) end )
end

function M.findFiles( root, destFile )
    assert(os.execute(string.format( 'find -L %q -type f > %q', root, destFile )))
end

function M.makeTable( dbconn, tbname, tags )
    assert( dbconn.exec( string.format(dcm.schema( tags or dcm.tags.all ), tbname) ) )
end

function M.getValues( dbconn, tbname )
    local hdr = dbconn.header( tbname )
    return fd.map( function(img) return fd.reduce( hdr, fd.map(function(k) return img[k] or '' end), fd.into, {}) end )
end

function M.getPath(dest, keys, x)
    local ret = fd.reduce( keys, fd.map( function(k) return tonumber(x[k]) and x[k] or x[k]:gsub('%s+$', ''):gsub('^%s+', ''):gsub('[^%w]', ''):gsub('%s', '_') end ), fd.into, { dest } )
    return table.concat( ret, '/' )
end

function M.mkdirs( dest, keys, x)
    local ret = fd.reduce( keys, fd.map( function(k) return tonumber(x[k]) and x[k] or x[k]:gsub('%s+$', ''):gsub('^%s+', ''):gsub('[^%w]', ''):gsub('%s', '_') end ), fd.into, { dest } )
    assert( os.execute('mkdir -p ' .. table.concat( ret, '/') ) )
end

return M
