
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

local CACHE	= cache'Hi vers'

local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--------------------------------
--
-- XXX store week & vers already received so info doesn't need to be generated once again
-- find all updates that need to be sent to a specific peer & send them all

local function adjust(fruit, week, vers)
    exec(format('%s/dump-stream.lua %s %s %d', APP, fruit, week, vers))
end

local function dumpPRICE() exec(format('%s/dump-price.lua', APP)) end

local function setVersion(v)
    if v == CACHE.has('vers') then return v else
	CACHE.store('vers', 'version ' .. v)
	print( v )
	return v
    end
end

local function switch( msg )

    local cmd = msg[1]

    if cmd == 'CACHE' then
	local fruit = msg[2]
	return CACHE.cache( fruit ) -- returns a table
    end

    if cmd == 'version' then
	return CACHE.has('vers')

 -- notification from 'weekdb' of an update returns a 'version' msg
    elseif cmd == 'update' then
	msg = msg[#msg] -- vers as json
	return 'version ' .. setVersion(msg)

    end

end

do
    dumpPRICE()
    local f = popen(format('%s/dump-vers.lua', APP))
    local v = f:read('l'):gsub('%s+%d$', '')
    f:close()
    setVersion(v)
    dump(DEST, v)
end


return switch
