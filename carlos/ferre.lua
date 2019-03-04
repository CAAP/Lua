local M = {}
-- Import Section
--
local file_exists  = require'carlos.bsd'.file_exists

local format 	   = require'string'.format
local connect	   = require'carlos.sqlite'.connect
local sleep	   = require'lbsd'.sleep

local assert	   = assert

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

--------------------------------
-- Local function definitions --
--------------------------------
--

--------------------------------
-- Public function definitions --
--------------------------------
--
-- accepts name of db and adds absolute path,
-- creating it if required, otherwise it checks
-- whether path already exists, always returning
-- the sql connection or error
function M.dbconn(path, create)
    local f = format(path)
    if create or file_exists(f) then
	return connect(f)
    else
	return false, format('ERROR: File %q does not exists!', f)
    end
end

function M.connexec( conn, s ) return assert( conn.exec(s) ) end

function M.ssevent( event, data ) return format('event: %s\ndata: %s\n\n', event, data) end

function M.decode(msg)
    local cmd = msg:match'%a+'
    local data = {}
    for k,v in msg:gmatch'(%a+)=([^=&]+)' do data[k] = v end
    return cmd, data
end

-- Maybe should be in "ferre-server" where it is needed!!!XXX
function M.chunks(f)
    local k = 1
    return function(x)
	f(x)
	if k%100 == 0 then sleep(1) end
	k = k + 1
    end
end

return M
