
-- Import Section
--
local fd	= require'carlos.fold'

local cache	= require'carlos.ferre'.cache

local format	= require'string'.format

local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local PINS	 = cache'Hi PINS'

local PIDS	 = {}
local FRUITS	 = {}

--------------------------------
-- Local function definitions --
--------------------------------
--
 -- cmds: tabs, delete, msgs
local function update(pid, cmd, msg)
    if not PIDS[pid] then PIDS[pid] = {} end
    local p = PIDS[pid]
    if cmd == 'delete' and p.tabs then p.tabs = nil
    else p[cmd] = '%s ' .. msg end -- add fruit
end

local function switch( msg )

    local cmd = msg:match'%a+'
    if cmd == 'CACHE' then
	local fruit = msg:match'%s(%a+)'
	return PINS.cache( fruit ) -- returns a table
    end
    local pid = msg:match'pid=(%d+)'
    if cmd == 'pins' then
	PINS.store(pid, msg)
	return 'OK'
    elseif cmd == 'login' then
	local fruit = msg:match'fruit=(%a+)'
	local ft = FRUITS[pid]
	local ret = {}
	if ft and ft ~= fruit then ret[1] = format('%s logout pid=%d', ft, pid) end
	FRUITS[pid] = fruit
	if PIDS[pid] then
	    fd.reduce(fd.keys(PIDS[pid]), fd.map(function(m) return format(m, fruit) end), fd.into, ret)
	end
	return ret -- returns a table
    else -- tabs, delete, msgs
	update(pid, cmd, msg)
	local ft = FRUITS[pid]
	if cmd == 'msgs' and ft then return format('%s %s', ft, msg) end
    end
--    print(msg, '\n')
--    ::FIN::

end

return switch
