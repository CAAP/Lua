--module setup
local M = {}

-- Import section
local lsql = require'lsql'
local str = require'carlos.string'
local fd = require'carlos.fold'

local ipairs = ipairs
local print = print
local error = error
local assert = assert
local tostring = tostring

local table = table
local string = string

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)
local noTable = "no such table:"
local isTable = "SELECT * FROM %q LIMIT 1"
local newTable = "CREATE TABLE IF NOT EXISTS %q ( %s )"
local newView = "CREATE VIEW IF NOT EXISTS %q %s"
local newIndex = "CREATE INDEX IF NOT EXISTS %s"
local inTable = "INSERT INTO %q VALUES( %s )"
local upTable = "UPDATE %q SET %s = ? WHERE %s LIKE ?"
local rmTable = "DELETE FROM %q WHERE %s = ?"
local mquery = [[
	SELECT sql, name, tbl_name, rowid FROM sqlite_master
	WHERE type != 'meta' AND sql NOTNULL AND name NOT LIKE 'sqlite_%'
	ORDER BY rowid
    ]]

-- Local function for module-only access

-- it can handle one or several records at a time
-- dynamically call query function
local function into( qryfn, args )
    return function( conn )
	local insert = conn.sink( qryfn, args )
	return function(x, i)
	    local done, msg = insert( x )
	    if not done then error( msg ) end
	end
    end
end

local function header( schema )
    local ret = schema:sub( schema:find'%(' + 1, -2 )
    local ans = fd.reduce(str.split( ret, ',', true ), fd.map( function(x) return x:match'%g+':gsub('"','') end), fd.into, {})
    return ans
end

local function connect( dbname )
    local conn

    if dbname == ":inmemory:" then conn = assert( lsql.inmem( ) )
    elseif dbname == ":temporary:" then conn = assert( lsql.temp( ) )
    else conn = assert( lsql.connect( dbname ) ) end

    local MM = {}

    local function exists( tbname ) return conn:prepare( string.format(isTable, tbname) ) end

    local function rows( query ) return function() return conn:rows( query ) end end

    MM.exists = exists -- XXX returns prepared statement ???

    MM.query = rows

    function MM.inQuery( tbname )
	local nCols = #assert(exists(tbname))
	return string.format(inTable, tbname, string.rep('?', nCols, ', ') )
    end

    function MM.upQuery( args )
	local tbname = args[1]
	assert(exists(tbname))
	return string.format(upTable, tbname, args[2], table.concat(args[3], ' = ? AND ') )
    end

    function MM.rmQuery( args )
	local tbname = args[1]
	assert(exits(tbname))
	return string.format(rmTable, tbname, table.concat(args[2], ' = ? AND ') )
    end

    function MM.header( tbname )
 	local schema = fd.first( rows( mquery ), function(x) return x.name:find( tbname ) end )
	assert( schema, 'Table not found.' )
	return header( schema.sql )
    end

    function MM.tables()
	return fd.reduce( rows( mquery ), fd.map( function(x) return x.name end ), fd.into, {} )
    end

    function MM.exec( query )
	local q = assert( conn:prepare( query ) )
--	print('Executing:', q,'\n')
	return conn:exec( q )
    end

    function MM.count( tbname, clause )
	if exists( tbname ) then
	    local qry =  string.format('SELECT COUNT(*) cnt FROM %q %s', tbname, clause or '')
            return fd.first( rows( qry ), function(x) return x.cnt end ).cnt
	else return 0 end
    end

    function MM.sink( qryfn, args )
	local stmt = MM[qryfn]( args )
	return conn:sink( stmt )
    end

    -- IN MEMORY DATABASES -- XXX correct so do not need to check for this!!!
    if dbname == ":inmemory:" or dbname == ":temporary:" then
	function MM.backup( dbpath, steps )
	    return lsql.backup(conn, dbpath, steps)
	end
    end

    function MM.info() return tostring(conn) end

    function MM.close()
	conn:close()
	MM = nil
    	return true
    end

    return MM 
end

---------------------------------
-- Public function definitions --
---------------------------------

M.connect = connect

----------------------------------

M.newTable = newTable

M.newView = newView

M.newIndex = newIndex

function M.into( tbname )
    return into( 'inQuery', tbname )
end

function M.update( args )
    return into( 'upQuery', args )
end

function M.delete( args )
    return into( 'rmQuery', args )
end

function M.query( dbname, query )
    return connect( dbname ).query( query )
end

function M.header( dbname, tbname )
    return connect( dbname ).header( tbname )
end

function M.exec( dbname, query )
    return connect( dbname ).exec( query )
end

function M.import(dbname, tbname, data, schema)
	assert( #data > 0 , 'Empty data.')

	local vals = data -- assume data contains only values
	local db = lsql.connect(dbname)
	local stmt, msg = db:prepare( string.format(isTable, tbname) )

	if not(stmt) then
		if msg:find( noTable ) then
			if schema then
				stmt = assert( db:prepare( string.format(schema, tbname) ))
			else
				print('Assuming first row as table schema.')
--				vals = {} -- data contains header in first row; it will re-populated
				local header = data[1]
				for i=1,#header do header[i] = string.format('%q', header[i]) end
				assert( #header > 0, 'Create table error: empty header.' )
				stmt = assert( db:prepare( string.format(newTable, tbname, table.concat(header, ' TEXT, ')) ))
				vals = fd.drop( 1, data, fd.into, {} ) -- data contains header in first row; it will re-populated
--				fd.reduce( data, function(x, i) if i>1 then vals[i-1] = x end end )
			end
			assert( db:exec( stmt ) ) -- execute CREATE TABLE
			stmt = assert( db:prepare( string.format(isTable, tbname) )) -- assert TABLE exists
			print('Table', tbname, 'created successfully.')
		else error( msg, 2 ) end
	end

	local nCols = #stmt
	assert( nCols > 0, 'Table error: empty table.')

	--  Table exists, let's populate it
	if #vals == 0 then print( '\nImport data none: empty dataset.' ); return end
	stmt = assert( db:prepare( string.format(inTable, tbname, string.rep('?', nCols, ', ')) ))
	print(stmt, '\n')
	print(#vals, 'rows will be inserted.\n')
	msg = db:import( stmt, vals )
	print( msg )
end

return M
