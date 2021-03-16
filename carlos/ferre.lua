local M = {}
-- Import Section
--
local fd = require'carlos.fold'

local file_exists  = require'carlos.bsd'.file_exists
local connect	   = require'carlos.sqlite'.connect
local reduce	   = require'carlos.fold'.reduce
local keys	   = require'carlos.fold'.keys
local into	   = require'carlos.fold'.into
local asJSON	   = require'json'.encode -- require'carlos.json'.asJSON
local dN	   = require'binser'.deserializeN
local sN	   = require'binser'.serialize
local fb64	   = require'lbsd'.fromB64
local b64	   = require'lbsd'.asB64
local md5	   = require'lbsd'.md5
local dump	   = require'carlos.files'.dump
local sleep	   = require'lbsd'.sleep
local env	   = os.getenv

local format 	   = string.format
local char	   = string.char
local tonumber	   = tonumber
local tointeger    = math.tointeger
local open	   = io.open
local popen	   = io.popen
local time	   = os.time
local date	   = os.date
local assert	   = assert
local pcall	   = pcall
local pairs	   = pairs
local concat	   = table.concat
local print	   = print
local type	   = type
local pcall	   = pcall

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local HOME	   = env'APP'
local ROOT	   = HOME .. '/ventas/json'
local DEST	   = ROOT .. '/version.json'

local SEMANA	   = 3600 * 24 * 7
local QUERY	   = 'SELECT * FROM updates %s'

local DICT	   = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
			A='K', B='L', C='M', D='N', E='O', F='P', G='Q',
			H='R', I='S', J='T', K='U', L='V', M='W', N='X',
			O='Y', P='Z', Q='a', R='b', S='c', T='d', U='e',
			V='f', W='g', X='h', Y='i', Z='j'}

--------------------------------
-- Local function definitions --
--------------------------------
--
local function hex(h) return char(tonumber(h, 16)) end

local function aspath(s) return format('%s/db/%s.db', env'HOME' , s) end

local function asnum(s) return (tointeger(s) or tonumber(s) or s) end

local function now() return time() end -- -21600  3.mx.pool.ntp.org

local function asweek(t) return date('Y%YW%U', t) end

local function dbconn(path, create)
    local f = aspath(path)
    if create or file_exists(f) then
	return connect(f)
    else
	return false, format('ERROR: File %q does not exists!', f)
    end
end

--
-- Functions to compute the current/ongoing version
-- based on the latest WEEK file on existence
--
local function backintime(week, t) while week < asweek(t) do t = t - 3600*24*7 end; return t end

-- Functions to 
--
-- remove 'vers' since it's an extra event in itself and add arg 'store'
--    w.vers = nil
local function prepare(w) w.store = 'PRICE'; return w; end

local function smart(v) return tointeger(v) or tonumber(v) or v:gsub('"',''):gsub("'",'') end

local function groupMe( a )
    return function(x)
	local k = smart(x.clave)
	if not a[k] then a[k] = {clave=k} end
	local v = a[k]
	v[x.campo] = smart(x.valor)
    end
end

local function asdata(conn, clause, week)
    local data = fd.reduce(conn.query(format(QUERY, clause)), groupMe, {})
    data = fd.reduce(fd.keys(data), fd.map(prepare), fd.into, {})
    data[#data+1] = {vers=conn.count('updates'), week=week, store='VERS'}
--    return asJSON(data)
    return data
end

local function fromWeek(week, vers)
    local conn =  dbconn(week)
    local vers = asnum(vers)
    local clause = vers > 0 and format('WHERE vers > %d', vers) or ''
    local N = conn.count('updates', clause)

    if N > 0 then return asdata(conn, clause, week) end
end

-- ITERATIVE procedure AWESOME
local function nextWeek(t) return {t=t+SEMANA, vers=0} end

local function stream(week, vers)
    local WKK = asweek(now())
    local function iter(wk, o)
	local w = asweek( o.t )
	if w > wk then return nil
	else
	    local q = fromWeek(w, o.vers)
	    if not q then return iter(WKK, nextWeek(o.t))
	    else return nextWeek(o.t), fromWeek(w, o.vers) end end
    end
    return function() return iter, WKK, {t=backintime(week, now()), vers=vers} end
end

local function send(srv, id, msg) return pcall(function() return srv:send_msgs{id, msg} end) end

local function tofruit( fruit, m ) return format('%s %s', fruit, m) end

--------------------------------
-- Public function definitions --
--------------------------------
--
-- accepts name of db (adds absolute path),
-- creating it if required, otherwise it checks
-- whether path already exists, always returning
-- the sql connection or error

M.HOME	 = HOME

M.APP	 = env'HOME' .. '/app-ferre'

M.aspath = aspath

M.now	 = now

M.asnum  = asnum

M.asweek = asweek

M.dbconn = dbconn

M.stream = stream

--M.version = version

M.asdata = asdata

function M.newUID() return date('%FT%TP', now()) end

function M.connexec( conn, s ) return assert( conn.exec(s) ) end

function M.ssevent( event, data )
    if event == 'SSE' then return data
    else return format('event: %s\ndata: %s\n\n', event, data) end
end

function M.uid2week( uid )
    local Y, M, D = uid:match'(%d+)%-(%d+)%-(%d+)T'
    return asweek(time{year=Y, month=M, day=D})
end

function M.decode(msg)
    local cmd = msg:match'%a+'
    local data = {}
    for k,v in msg:gmatch'(%a+)=([^=&]+)' do data[k] = asnum(v) end
    return cmd, data
end

function M.urldecode(s) return s:gsub('+', '|'):gsub('%%(%x%x)', hex) end -- :gsub('&', '|')

function M.receive(srv, waitp)
    local id, more = srv:recv_msg()
    return id, more and srv:recv_msgs(waitp) or {}
end

function M.deserialize(s)
    local a,i = dN(fb64(s), 1)
    return a
end

local function serialize(o) return b64(sN(o)) end

M.serialize = serialize

function M.catchall(w, skt, cmd, msg, a)
    if type(w[cmd]) == 'function' then
	local done, err = pcall(w[cmd], skt, msg)
	if not done then
	    a[#a] = serialize{cmd='errorx', data=msg, msg=err}
	    skt:send_msgs(a)
	end

    else
	a[#a] = serialize{cmd='errorx', data=msg, msg='function does not exists'}
	skt:send_msgs(a)

    end
end

function M.digest(oldv, newv)
    assert(oldv ~= newv, "EEROR: versions must be different")
    if oldv > newv then oldv, newv = newv, oldv end
    return b64(md5(md5(oldv)..md5(newv)))
end

-- DUMP
function M.dumpFEED(conn, PATH, qry)
    local FIN = open(PATH, 'w')
    FIN:write'['
    FIN:write( concat(fd.reduce(conn.query(qry), fd.map(getName), fd.map(asJSON), fd.into, {}), ',\n') )
    FIN:write']'
    FIN:close()
    return 'Updates stored and dumped'
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
    local function snd2fruit(fruit) return function(m) return tofruit(fruit, m) end end

    function MM.store(pid, msg) CACHE[pid] = msg end

    function MM.delete( pid ) CACHE[pid] = nil end

    function MM.cache(fruit) return reduce(keys(CACHE), fd.map(snd2fruit(fruit)), fd.into, {}) end

--    function MM.sndkch(msgr, fruit) reduce(keys(CACHE), function(m) msgr:send_msg(tofruit(fruit, m)) end) end

    function MM.has( pid ) return CACHE[pid] end

    return MM
end

M.send = send

function M.getFruit(active)
    local k = fd.first(FRUITS, function(k) return not(active[k]) end)
    return k or 'orphan' -- and k:match'[a-z]+' and k 
end

function M.queryDB(msg)
    local fruit = msg:match'fruit=(%a+)'
    msg = msg:match('%a+%s([^!]+)'):gsub('&', '!')
--    print('Querying database:', msg, '\n')
    local f = assert( popen(format('%s/dump-query.lua %s', M.APP, msg)) )
    local v = f:read'l'
    f:close()
    return format('%s query %s', fruit, v)
end

return M


--[[
-- if db file exists and 'updates' tb exists then returns count
local function which( db )
    local conn = assert( dbconn( db ) )
    if conn and conn.exists'updates' then
	return conn.count'updates'
    else return 0 end
end

local function version()
    local hoy = now()
    local week = asweek( hoy )
    local vers = which( week )
    while vers == 0 do -- change in YEAR XXX
	hoy = hoy - SEMANA
	week = asweek( hoy )
	vers = which( week )
--	if week:match'W00' then break end
    end
    return asJSON{week=week, vers=vers}
end
--]]


