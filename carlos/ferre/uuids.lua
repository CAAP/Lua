
-- Import Section
--
local asnum	= require'carlos.ferre'.asnum
local newUID	= require'carlos.ferre'.newUID

local tointeger = math.tointeger
local concat	= table.concat
local assert	= assert
local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local IDS	= 'app:uuids:'

local CACHE	= {}

--------------------------------
-- Local function definitions --
--------------------------------
--
local function asUUID(client, cmd, msg)
    local pid = asnum(msg:match'pid=([%d%a]+)')
    local uuid = msg:match'uuid=(%w+)'
    local length = tointeger(msg:match'length=(%d+)')
    local size = tointeger(msg:match'size=(%d+)')
    local msg = msg:sub(msg:find'query=', -1) -- leaving only query=ITEM_1&query=ITEM_2&query=ITEM_3...
    -- XXX urldecode should go here

    if uuid then
	if not(client:exists(IDS..uuid)) then
	    client:hset(IDS..uuid, 'uid', newUID()..pid, 'cmd', cmd, 'pid', pid)
	    client:expire(IDS..uuid, 60) -- *VOLATILE*
	    CACHE[uuid] = {}
	end
	local w = CACHE[uuid]
	if length <= (#w * size) then
	    client:hset(IDS..uuid, 'data', concat(CACHE[uuid], '&'))
	    CACHE[uuid] = nil
	    return uuid
	else return false end

    else
	client:hset(IDS..pid, 'uid', newUID()..pid, 'cmd', cmd, 'pid', pid, 'data', msg)
	client:expire(IDS..pid, 60) -- *VOLATILE*
	return pid

    end
end

return asUUID
