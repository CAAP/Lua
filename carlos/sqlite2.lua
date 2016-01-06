--module setup
local M = {}

-- Import section
local sql = require'lsqlite3'
local ipairs = ipairs
local table = table
local print = print
local error = error

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

local function statement(tb, N)
	local ret = {}
	for i=1,N do
		ret[#ret+1] = '?'
	end
	local ans = table.concat(ret, ',')
	return 'INSERT INTO '..tb..' VALUES( '..ans..')'
end

-- Local function for module-only access

local function prepared(db, sql_stmt)
	local stmt = db:prepare( sql_stmt )
	local MM = {}

	function MM.one(value)
		local ret = (stmt:bind(value) == sql.OK and stmt:step() == sql.DONE)
		stmt:finalize()
		return ret
	end

	function MM.bindMany(atable)
		local ret = (stmt:bind_values( table.unpack(atable) ) == sql.OK and stmt:step() == sql.DONE)
		stmt:finalize()
		return ret
	end

	function MM.multiple(atable)
		local ret = {}
		for i,x in ipairs(atable) do
			if not( stmt:bind_values( table.unpack(x) ) == sql.OK and stmt:step() == sql.DONE ) then ret[#ret+1] = i end 
			stmt:reset()
		end
		stmt:finalize()
		print(#atable-#ret,'rows inserted successfully.')
		return #ret == 0, ret
	end

	if stmt then return MM end
end

---------------------------------
-- Public function definitions --
---------------------------------

function M.import(dbname, tbname, data, schema)
	local db = sql.open(dbname)

	local function exit_error(msg)
		db:close()
		error(msg)
	end

	if schema then
		local stmt = schema:gsub('?', tbname)
		print(stmt,'\n')
		if not(db:exec( stmt ) == sql.OK) then exit_error'Error creating table' end
	end

	local stmt = statement(tbname, #data[1])
	print(stmt, '\n')
	stmt = prepared(db, stmt)
	if not(stmt) then exit_error'Error parsing schema.' end
	local ok, err = stmt.multiple( data )
	if not(ok) then print('Error parsing rows: '.. table.concat(err, ', ')) end
	
	db:close()
end

return M
