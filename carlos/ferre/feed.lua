
-- Import Section
--
local reduce	= require'carlos.fold'.reduce
local map	= require'carlos.fold'.map
local into	= require'carlos.fold'.into

local asweek	= require'carlos.ferre'.asweek
local dbconn	= require'carlos.ferre'.dbconn
local now	= require'carlos.ferre'.now
local newUID	= require'carlos.ferre'.newUID

local asJSON	= require'json'.encode

local format	= require'string'.format

local print	= print
local assert	= assert

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local MEM	 = {}

local QUID	 = 'SELECT uid, SUBSTR(uid, 12, 5) time, SUM(qty) count, ROUND(SUM(totalCents)/100.0, 2) total, tag FROM tickets WHERE tag NOT LIKE "factura" AND uid %s %q GROUP BY uid'

--------------------------------
-- Local function definitions --
--------------------------------
--

local function route( fruit ) return function(m) return format('%s %s', fruit, m) end end

local function store( msg )
--    if #MEM > 500 then MEM = {} end
    MEM[#MEM+1] = msg
    return msg
end

local function switch( cmd, msg )

    if cmd == 'CACHE' then
	local fruit = msg:match'%a+'
	return reduce(MEM, map( route(fruit) ), into, {})
    end

    if cmd == 'feed' then
	msg = msg[2]
	if msg:match'uid' then return store( msg ) else
	return MEM end
    end

end

do
    local TODAY = newUID():match'([%d%-]+)T' .. '%'
    local WEEK = assert( dbconn( asweek( now() ) ) )
    local qry = format(QUID, 'LIKE', TODAY)
    reduce(WEEK.query(qry), map(asJSON), into, MEM)
end


return switch
