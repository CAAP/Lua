--module setup
local M = {}

-- Import section
local dcm = require'gdcm'
local sql = require'lsqlite3'
local fs = require'carlos.files'
local tb = require'carlos.tables'
local math = math
local table = table
local ipairs = ipairs
local print = print
local tonumber = tonumber
local assert = assert

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)
local uid =  {0x0020, 0x000e} -- series instance UID
local id =   {0x0010, 0x0020} -- patient ID
local srn =  {0x0020, 0x0011} -- series number
local num =  {0x0020, 0x0013} -- image number
local mdty = {0x0008, 0x0060} -- modality
local stdy = {0x0008, 0x1030} -- study description
local srs =  {0x0008, 0x103E} -- series description
local ppos = {0x0018, 0x5100} -- patient position
local rows = {0x0028, 0x0010} -- rows
local date = {0x0008, 0x0020} -- study date
local thick= {0x0018, 0x0050} -- slice thickness
local resol= {0x0028, 0x0030} -- resolution: dx, dy
local ipp =  {0x0020, 0x0032} -- image position patient
local iop =  {0x0020, 0x0037} -- image orientation patient

local all = {uid, id, srn, num, mdty, stdy, srs, ppos, rows, date, thick, resol, ipp}
local some = {uid, id, num, mdty}

local schema = [[
	CREATE TABLE variables (
	    path STRING,
	    UID STRING,
	    PID NUMBER,
	    series_number INTEGER,
	    image_number INTEGER,
	    modality STRING,
	    study_description STRING,
	    series_description STRING,
	    patient_position STRING,
	    rows INTEGER,
	    study_date DATE,
	    slice_thickness INTEGER,
	    resolution STRING,
	    IPP STRING,
	    dx NUMBER,
	    dy NUMBER,
	    dz NUMBER,
	    grid_slice INTEGER,
	    grid_cell INTEGER,
	    SID INTEGER);
]]

local statement = [[
	INSERT INTO variables VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
]]

local width = 4
local height= 5

-- Local function for module-only access
local function generate(path)
	local data = dcm.scanner(path, all)
	data = tb.filter(data, function(x) return x[2] and x[3] and x[5] and x[13] end)
	table.sort(data, function(x, y) return x[2] == y[2] and x[5] > y[5] or (x[3] == y[3] and x[2] < y[2] or x[3] < y[3]) end)

	local wh = width*height
	local series = math.random(1000, 9000)
	local z1 = 0
	local digit = '(%-?%d+%.%d+)'
	for _,x in ipairs(data) do
		local dx, dy = x[13]:match(digit..'\\'..digit) -- resolution
		local _,_,z2 = x[14]:match(digit..'\\'..digit..'\\'..digit) -- ipp
		local slice = math.ceil( x[5] / wh )
		local cell = x[5] - (slice-1)*wh
		z2 = tonumber(z2) or 0
		local dz = math.abs(z1 - z2)

		x[#x+1] = dx
		x[#x+1] = dy
		x[#x+1] = dz
		x[#x+1] = slice
		x[#x+1] = cell
		x[#x+1] = series

		z1 = z2
		if x[5] == 1 then series = math.random(1000, 9000) end
	end
	
	return data
end

---------------------------------
-- Public function definitions --
---------------------------------

M.all = all

M.schema = schema

M.some = some

function M.scanner(path)
	return dcm.scanner(path, all)
end

function M.header(path)
	return dcm.header(path, all)
end

function M.byIDdesc(files)
	table.sort(files, function(x, y) return x[2] == y[2] and x[5] > y[5] or (x[3] == y[3] and x[2] < y[2] or x[3] < y[3]) end)
end

function M.dump2csv(path, csvfile)
	local data = generate(path)
	fs.dump(csvfile, tb.asstr(data, '\t'))
	fs.dump(csvfile:gsub('csv$','sql'), schema)
end

function M.dump2sql(path, sqldb)
	local data = generate(path)
	local db = sql.open(sqldb)

	assert( db:exec( schema ) == sql.OK, 'Error creating TABLE variables')

	local stmt = assert( db:prepare(statement), 'Error creating STATEMENT.')
	for _,x in ipairs(data) do
		if stmt:bind_values( table.unpack(x) ) == sql.OK then
			if stmt:step() ~= sql.DONE then print('Error evaluating file: ', x[1]) end
		else print('Error parsing file: ', x[1]) end
		stmt:reset()
	end
	stmt:finalize()

	print'DB generated successfully.'
	db:close()
end


return M
