-- Import Section
--
local asnum	  = require'carlos.ferre'.asnum
local newUID	  = require'carlos.ferre'.newUID
local deserialize = require'carlos.ferre'.deserialize
local serialize   = require'carlos.ferre'.serialize

local format	= string.format
local tointeger = math.tointeger
local concat	= table.concat
local assert	= assert
local print	= print

local TT	= os.getenv'TIENDA':match'%d+$'

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local IDS	= 'app:uuids'

local CACHE	= {}

--------------------------------
-- Local function definitions --
--------------------------------
--

local function UID(pid) return format('%s%d:%s', newUID(), pid, TT) end

local function asUUID(client, s)
    local w = deserialize(s)
    local uuid = w.uuid
    local pid = tointeger(w.pid)
    local count = w.count or 0

    assert( uuid and (count > 0) )

    -- short circuit --
    if count == 1 then
	local uid = UID(pid)
	w.uid = uid
	client:hset(IDS, uuid, serialize{w}) -- serialized table of objects
	return uuid, uid, pid
    end

    if not(client:hexists(IDS, uuid)) then
	client:hset(IDS, uuid, true) -- placeholder
	CACHE[uuid] = {}
	CACHE[pid] = UID(pid) -- computed only once
    end

    w.uid = CACHE[pid] -- set uid in object
    local q = CACHE[uuid]
    q[#q+1] = w -- keep deserialized object
    if count == #q then
	client:hset(IDS, uuid, serialize(q)) -- serialized table of objects
	CACHE[uuid] = nil
	CACHE[pid] = nil
	return uuid, w.uid, pid

    else
	return false

    end
end

return asUUID
