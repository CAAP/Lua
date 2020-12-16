
-- Import Section
--

local redis	= require'redis'
local ssevent	= require'carlos.ferre'.ssevent

local assert	= assert
local print	= print

local SSE	= os.getenv'SSE_PORT'

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

local client	 = assert( redis.connect('127.0.0.1', '6379') )

local ESTREAM	 = assert( client:get'tcp:sse' )
local MG	 = 'mgconn:active'
local AP	 = 'app:active'

local events

--------------------------------
-- Local function definitions --
--------------------------------
--

local function conn2fruit( c )
    local fruit = client:rpoplpush('const:fruits', 'const:fruits')
    c:set_id(fruit)
    client:sadd(MG, fruit)
    return fruit
end

local function connectme( c )
    local fruit = assert( c:id() )
    c:send(ESTREAM)
    c:send'\n\n'
    c:send( ssevent('fruit', fruit) )
    c:send( ssevent('version', client:get'app:updates:version') )
end

local function sayoonara( c )
    local fruit = assert( c:id() )
    local pid = client:hget(AP, fruit) or 'NaP'
    client:srem(MG, fruit)
    client:hdel(AP, fruit, pid)
    return fruit
end

local function backend(c, ev, ...)
    if ev == events.ACCEPT then
	local fruit = conn2fruit(c)
	print('\n+\n\nSSE\tNew fruit:', fruit, '\n')

    elseif ev == events.HTTP then
	connectme(c)
	print'\tconnection established\n\n+\n'

    elseif ev == events.CLOSE then
	print('\n+\nSSE\tbye bye', sayoonara(c), '\n+')

    end
end

local function init(mgr)
    events = mgr.events
    return mgr.bind('http://localhost:'..SSE, backend, 'http'), SSE
end

return init
