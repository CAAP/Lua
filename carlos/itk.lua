-- module setup
local M = {}

-- Import Section
local assert = assert
local ipairs = ipairs
local pairs = pairs
local print = print
local string = string

local fd = require'carlos.fold'

local litk = require'litk'


-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access


--------------------------------
-- Local function definitions --
--------------------------------

local function dicoms(vfs, i)
print('Processing file path: ', vfs[1])
    local img = litk.ushortNew()
print('Image container created successfully.')
    img:dicom(vfs)
print('Dicom files imported successfully.')
    local p = string.format("%05d.vtk", i)
print(p, '\n')
    img:write(p)
end

---------------------------------
-- Public function definitions --
---------------------------------

function M.dcmSeries( path )
    local ss = litk.series( path )
    local uids = ss:uids()
    local fs = fd.reduce( uids, fd.map(function(s) return ss:files(s) end), fd.filter(function(v) return #v > 10 end), fd.into, {} )
    local ps = fd.reduce( fs, fd.map(function(v) return litk.pixelType(v[1]) end), fd.into, {} )
    fd.reduce( fs, function(vfs,j) if ps[j] == 'ushort' then dicoms(vfs,j) end end )
end

return M
