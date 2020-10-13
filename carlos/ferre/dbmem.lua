
-- Import Section
--
local connect	= require'carlos.sqlite'.connect
local uid2week	= require'carlos.ferre'.uid2week
local aspath	= require'carlos.ferre'.aspath

local format	= string.format

local assert	= assert

-- No more external access after this point
_ENV = nil -- or M

local M 	= {}
-- Local Variables for module-only access
--
local DB	= {}

--------------------------------
-- Local function definitions --
--------------------------------
--
local function addDB(week, ups)
    local conn = connect':inmemory:'
    DB[week] = conn
    assert( conn.exec(format('ATTACH DATABASE %q AS week', aspath(week))) )
    assert( conn.exec'CREATE TABLE tickets AS SELECT * FROM week.tickets' )
    if ups then
	assert( conn.exec'CREATE TABLE updates AS SELECT * FROM week.updates' )
    end
    assert( conn.exec'DETACH DATABASE week' )
    assert( conn.exec(SUID) )
    assert( conn.exec(SLPR) )
    assert( conn.exec(SALES) )
    return conn
end

--[[
local function tryDB(conn, week, vers)
    local k = vers > 0 and 'WHERE vers > '.. vers or ''
    local qry = format('INSERT INTO messages SELECT msg FROM week.updates %s', k)
    assert( conn.exec(format('ATTACH DATABASE %q AS week', aspath(week))) )
    assert( conn.exec(qry) )
    assert( conn.exec'DETACH DATABASE week' )
end
--]]

function M.getConn(uid)
    local week = uid2week(uid)
    return DB[week] or addDB(week, false)
end

M.addDB = addDB

return M
