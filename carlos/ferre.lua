local M = {}
-- Import Section
--
local file_exists  = require'carlos.bsd'.file_exists

local format 	   = require'string'.format
local connect	   = require'carlos.sqlite'.connect
local reduce	   = require'carlos.fold'.reduce
local keys	   = require'carlos.fold'.keys
local into	   = require'carlos.fold'.into
local sleep	   = require'lbsd'.sleep
local env	   = os.getenv

local char	   = string.char
local tonumber	   = tonumber
local tointeger    = math.tointeger
local time	   = os.time
local date	   = os.date
local assert	   = assert

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

--------------------------------
-- Local function definitions --
--------------------------------
--
local function tofruit( fruit, m ) return format('%s %s', fruit, m) end

local function hex(h) return char(tonumber(h, 16)) end

local function aspath(s) return format('%s/db/%s.db', env'HOME', s) end

local function asnum(s) return (tointeger(s) or tonumber(s) or s) end

local function now() return time() end -- -21600  3.mx.pool.ntp.org

--------------------------------
-- Public function definitions --
--------------------------------
--
-- accepts name of db (adds absolute path),
-- creating it if required, otherwise it checks
-- whether path already exists, always returning
-- the sql connection or error

M.aspath = aspath

M.now	 = now

M.asnum  = asnum

function M.newUID() return date('%FT%TP', now()) end

function M.dbconn(path, create)
    local f = aspath(path)
    if create or file_exists(f) then
	return connect(f)
    else
	return false, format('ERROR: File %q does not exists!', f)
    end
end

function M.connexec( conn, s ) return assert( conn.exec(s) ) end

function M.ssevent( event, data )
    if event == 'SSE' then return data
    else return format('event: %s\ndata: %s\n\n', event, data) end
end

function M.asweek( uid )
    local Y, M, D = uid:match'(%d+)%-(%d+)%-(%d+)T'
    return date('Y%YW%U', time{year=Y, month=M, day=D})
end

function M.decode(msg)
    local cmd = msg:match'%a+'
    local data = {}
    for k,v in msg:gmatch'(%a+)=([^=&]+)' do data[k] = asnum(v) end
    return cmd, data
end

function M.urldecode(s) return s:gsub('+', '|'):gsub('%%(%x%x)', hex) end

function M.receive(srv)
    local function msgs() return srv:recv_msgs() end -- returns iter, state & counter
    local id, more = srv:recv_msg()
    if id and more then more = reduce(msgs, into, {}) end
    return id, more
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

-- XXX function 'send_msg' can optionally return IMMEDIATELY - NOWAIT flag
function M.cache(ps)
    local MM = {}
    local CACHE = {karl=ps}

    function MM.store(pid, msg) CACHE[pid] = msg end

    function MM.delete( pid ) CACHE[pid] = nil end

    function MM.sndkch(msgr, fruit) reduce(keys(CACHE), function(m) msgr:send_msg(tofruit(fruit, m)) end) end

    return MM
end

return M
