local M = {}
-- Import Section
--
local file_exists  = require'carlos.bsd'.file_exists

local format 	   = require'string'.format
local connect	   = require'carlos.sqlite'.connect
local sleep	   = require'lbsd'.sleep
local socket	   = require'socket'

local assert	   = assert

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

--------------------------------
-- Local function definitions --
--------------------------------
--

local function server(port)
    local srv = assert( socket.bind('*', port) )
    local skt = srv:getsockname()
    srv:settimeout(0)
    return srv, skt
--    print(skt, 'listening on port', port, '\n')
end

local function handshake(srv)
    local c = srv:accept()
    if c then
	c:settimeout(1)
	local ip = c:getpeername():match'%g+' --XXX ip should be used
--	print(ip, 'connected on port 8080 to', skt)
--	local response = hd.response({content='stream', body='retry: 60'}).asstr()
	if c:send( response ) and initFeed(c) then cts[#cts+1] = c
	else c:close() end
	end
end

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
