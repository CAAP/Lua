-- module setup
local M = {}

-- Import Section
local sql = require'carlos.sqlite'
local fd = require'carlos.fold'

local concat = table.concat
local format = string.format
local open = io.open
local asint = math.tointeger
local assert = assert
local pairs = pairs
local type = type
local tonumber = tonumber

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

--------------------------------
-- Local function definitions --
--------------------------------

local function asJSON( w )
    local ret = {}
    for k,v in pairs(w) do
	local u = type(v) == 'table' and asJSON(v) or (asint(v) or v)
	ret[#ret+1] = format('%q: %'..((tonumber(u) or u:match'"' or u:match'{') and 's' or 'q'), k, u)
    end
    return format('{%s}', concat(ret, ', ')):gsub('"%[', '['):gsub(']"', ']'):gsub("'", '"')
end

---------------------------------
-- Public function definitions --
---------------------------------

M.asJSON = asJSON

function M.escape(s) return s:gsub('"', '&quot;'):gsub('&', '&amp;') end

function M.sql2json( q )
    assert(open(q.dbname)) -- file MUST exists
    local db = assert(sql.connect(q.dbname), 'Could not connect to DB '..q.dbname)
    if db.count(q.tbname, q.clause) > 0 then
	local ret = fd.reduce( db.query( q.QRY ), fd.map( asJSON ), fd.into, {} )
	return format('Content-Type: text/plain; charset=utf-8\r\n\r\n[%s]\n', concat(ret, ', '))
    else
	return 'Content-Type: text/plain\r\n\r\n[]\n'
    end
end


return M

