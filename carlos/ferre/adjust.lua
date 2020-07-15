
-- Import Section
--
local fd	= require'carlos.fold'

local cache	= require'carlos.ferre'.cache

local format	= string.format
local insert	= table.insert
local concat	= table.concat
local unpack	= table.unpack

local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local PINS	 = cache'Hi PINS'
local TABS	 = cache'Hi TABS'
--TABS.tabs = TABS.store

local FRUITS	 = {}

--------------------------------
-- Local function definitions --
--------------------------------
--

TABS.tabs = function(pid, msg)
    local ret = TABS.has(pid)
    if ret then fd.drop(1, msg, fd.into, ret)
    else TABS.store(pid, msg) end
end

local function join(w, fruit)
    if w then
	local u = { unpack(w) }
	u[1] = format('%s %s', fruit, w[1])
	u[#u] = w[#w]..'\n\n'
	return concat(u, '&query=')
    end
end

-- CACHE sent is the PINS stored for each employee
-- tabs are only sent once a login succeeds
local function switch( cmd, pid, msg )
    if cmd == 'CACHE' then
	local fruit = msg[2]:match'(%a+)' -- %s?
	return PINS.cache( fruit ) -- returns a table
    end

    local ft = FRUITS[pid]

    -- short-circuit & re-route the message
    if cmd == 'msgs' and ft then
	return format('%s %s', ft, msg)

    -- store new PIN
    elseif cmd == 'pins' then
	PINS.store(pid, msg)
	return msg

    elseif cmd == 'login' then
	local fruit = msg:match'fruit=(%a+)'
	local ret = {}
	-- guard against double login by sending message to first session & closing it
	if ft and ft ~= fruit then ret[1] = format('%s logout pid=%d', ft, pid) end
	FRUITS[pid] = fruit -- new session opened & saved
	ret[#ret+1] = join(TABS.has(pid), fruit) -- tabs data, if any
	return ret -- returns a table | possibly empty

    else -- tabs, delete
	TABS[cmd](pid, msg)
	return 'OK'

    end

end

return switch
