
-- Import Section
--
local fd	= require'carlos.fold'

local cache	= require'carlos.ferre'.cache
local decode	= require'carlos.ferre'.decode
local dump	= require'carlos.files'.dump

local format	= require'string'.format
local popen	= io.popen
local exec	= os.execute

local HOME	= require'carlos.ferre'.HOME
local APP	= require'carlos.ferre'.APP
local DEST	= HOME .. '/json/version.json' -- /ventas

local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local CACHE	 = cache'Hi VERS'

--------------------------------
-- Local function definitions --
--------------------------------
--
-- find all updates that need to be sent to a specific peer & send them all
local function adjust(fruit, week, vers) exec(format('%s/dump-fruit.lua %s %s %d', APP, fruit, week, vers)) end

local function dumpPRICE() exec(format('%s/dump-price.lua', APP)) end

local function getVersion()
    dumpPRICE()

    local f = popen(format('%s/dump-vers.lua', APP))
    local v = f:read('l'):gsub('%s+%d$', '')
    f:close()

    dump(DEST, v) -- XXX really need this???

    CACHE.store('vers', format('version %s', v))
    print(v)
    return v
end

local function switch( msg, msgr )

    local cmd = msg:match'%a+'
    if cmd == 'CACHE' then
	local fruit = msg:match'%s(%a+)'
	CACHE.sndkch( msgr, fruit )
	print('version:\nCACHE sent to', fruit, '\n')
	goto FIN
    end

    if cmd == 'version' then
	print'Version event ongoing!\n'
	msgr:send_msg(format('version %s', getVersion()))

    elseif cmd == 'adjust' then
	local cmd, o = decode(msg) -- o: fruit, week, vers
	adjust(o.fruit, o.week, o.vers)
	print'Adjust process successful!\n'
	msgr:send_msg(format('%s adjust %s.json', o.fruit, o.fruit))
    end

    print(msg, '\n')
    ::FIN::

end

getVersion()

return switch
