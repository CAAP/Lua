
-- Import Section
--

local redis	= require'redis'
local json	= require'json'.encode

local assert	= assert
local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

local client	 = assert( redis.connect('127.0.0.1', '6379') )

local MG	 = 'mgconn:active'
local AP	 = 'app:active'

local ops

local MM = {}

--------------------------------
-- Local function definitions --
--------------------------------
--

local function isvalid( c ) return c:opt'accepted' and c:opt'websocket' and c:opt'label' end

local function conn2fruit( c )
    local fruit = client:rpoplpush('const:fruits', 'const:fruits')
    c:opt('label', fruit)
    client:sadd(MG, fruit)
    return fruit
end

local function sayoonara( c )
    local fruit = assert( c:opt'label' )
    local pid = client:hget(AP, fruit) or 'NaP'
    client:srem(MG, fruit)
    client:hdel(AP, fruit, pid)
    return fruit
end

local function connectme( c )
    local fruit = assert( c:opt'label' )
    c:send( json{cmd='fruit', fruit=fruit} )
--    c:send( json{title='version', msg=client:get'app:updates:version'} )
end

function MM.accept(c)
    local fruit = conn2fruit(c)
    print('\n+\n\nSSE\tNew fruit:', fruit, '\n')
    return fruit
end

function MM.http(c)
    connectme(c)
    print'\tconnection established\n\n+\n'
end

function MM.error(c, ...)
    print('ERROR', ...)
    c:opt('closing', true)
end

function MM.close(c)
    print('\n+\nSSE\tbye bye', sayoonara(c), '\n+')
end

MM.isvalid = isvalid

return MM



