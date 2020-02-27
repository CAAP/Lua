
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
local VERS	 = ''

--------------------------------
-- Local function definitions --
--------------------------------
--
-- XXX store week & vers already received so info doesn't need to be generated once again
-- find all updates that need to be sent to a specific peer & send them all
local function adjust(fruit, week, vers)
    exec(format('%s/dump-stream.lua %s %s %d', APP, fruit, week, vers))
end

local function dumpPRICE() exec(format('%s/dump-price.lua', APP)) end

local function getVersion()
    local f = popen(format('%s/dump-vers.lua', APP)) -- think about just reading a file instead XXX
    local v = f:read('l'):gsub('%s+%d$', '') -- like after changes in dbweek write the file directly XXX
    f:close()

    if v ~= VERS then
	VERS = v
	dump(DEST, v)
	dumpPRICE() -- XXX send a message to dbferre instead or the app should've done it already
	CACHE.store('vers', format('version %s', v))
    end

    return v
end

local function switch( msg )

    local cmd = msg:match'%a+'
    if cmd == 'CACHE' then
	local fruit = msg:match'%s(%a+)'
	return CACHE.cache( fruit ) -- returns a table
    end

    if cmd == 'version' then
	return getVersion()

    elseif cmd == 'adjust' then
	local cmd, o = decode(msg) -- o: fruit, week, vers
	adjust(o.fruit, o.week, o.vers)
	return format('%s adjust %s.json', o.fruit, o.fruit)
    end

end

print( getVersion() )

return switch
