
-- Import Section
--
local fd	= require'carlos.fold'
local dbconn	= require'carlos.ferre'.dbconn
local asnum	= require'carlos.ferre'.asnum
local ticket	= require'carlos.ticket'.ticket

local format	= require'string'.format

local popen	= io.popen

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

local PEOPLE = { A = 'caja' }

--------------------------------
-- Local function definitions --
--------------------------------
--


local function addName(o)
    local pid = asnum(o.uid:match'P([%d%a]+)')
    o.nombre = pid and PEOPLE[pid] or 'NaP';
    return o
end


local function bixolon( msg, PRINTER )
    local HEAD = {'tag', 'uid', 'total', 'nombre'}
    local DATOS = {'clave', 'desc', 'qty', 'rea', 'unitario', 'subTotal'}

    local head = addName(fd.first(conn.query(format(QHEAD, uid)), function(x) return x end))

    local data = ticket(head, fd.reduce(conn.query(format(QLPR, uid)), fd.into, {}))

    local skt = popen(PRINTER, 'w')
    if #data > 8 then
	data = fd.slice(4, data, fd.into, {})
	fd.reduce(data, function(v) skt:write(concat(v,'\n'), '\n') end)
    else
	skt:write( concat(data,'\n') )
    end
    skt:close()


end

--
-- Store PEOPLE values
--
do
    local people = assert( dbconn'personas' )
    fd.reduce(people.query'SELECT * FROM empleados', fd.rejig(function(o) return o.nombre, asnum(o.id) end), fd.merge, PEOPLE)
end
--


return bixolon
