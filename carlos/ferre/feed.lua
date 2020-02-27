
-- Import Section
--
local fd	= require'carlos.fold'

local format	= require'string'.format
local exec	= os.execute

local APP	= require'carlos.ferre'.APP

local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

--------------------------------
-- Local function definitions --
--------------------------------
--

-- XXX change fruit for a random STRING
local function dump(cmd, frt, uid)
    exec(format('%s/dump-feed.lua %s %s %s', APP, cmd, frt, uid))
end

-- XXX change fruit for a random STRING
local function sndmsg( cmd, fruit)
    return format('%s %s %s-feed.json', fruit, cmd, fruit)
end

local function switch( msg, msgr )

    local cmd = msg:match'%a+'
    if cmd == 'feed' then
	local fruit = msg:match'%s(%a+)'
	dump(cmd, fruit, '')
	msgr:send_msg( sndmsg(cmd, fruit) )

    elseif cmd == 'uid' then
	local fruit = msg:match'fruit=(%a+)'
	local uid   = msg:match'uid=([^!&]+)'
	dump(cmd, fruit, uid)
	msgr:send_msg( sndmsg(cmd, fruit) )

    elseif cmd == 'ledger' then
	local fruit = msg:match'fruit=(%a+)'
	local uid   = msg:match'uid=([^!&]+)'
	dump(cmd, fruit, uid)
	msgr:send_msg( sndmsg(cmd, fruit) )

    end

    print(msg, '\n')

end

return switch
