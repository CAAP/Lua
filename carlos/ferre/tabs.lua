
-- Import Section
--
local fd	  = require'carlos.fold'
local asUUID	  = require'carlos.ferre.uuids'
local deserialize = require'carlos.ferre'.deserialize
local serialize   = require'carlos.ferre'.serialize
local rconnect	  = require'redis'.connect

local format	  = string.format
local insert	  = table.insert
local concat	  = table.concat
local assert	  = assert
local print	  = print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local client	= assert( rconnect('127.0.0.1', '6379') )

local AP	= 'app:active'
local MM	= 'app:mem:'
local IDS	= 'app:uuids'

--------------------------------
-- Local function definitions --
--------------------------------
--
local function addFruit(fruit) 
    return function(w)
	w.fruit = fruit
	return serialize(w)
    end
end

 -- returns a table (possibly empty) | nil
local function tabs(cmd, s)
    local w = deserialize(s)
    local pid = w.pid
    local ft = client:hget(AP, pid)
    local k = MM..pid -- Memory/Cached key

    if cmd == 'login' then
	local fruit = w.fruit
	local ret = {}

	-- guard against double login by sending message to first session & closing it
	if ft and ft ~= fruit then
	    ret[1] = serialize{cmd='logout', fruit=ft, pid=pid}
	end

	-- in any case
	client:hset(AP, fruit, pid, pid, fruit) -- new session open
	print('\n\tSuccessful login for', fruit, '\n')

	if client:exists(k) then
	    local setFruit = addFruit(fruit)
	    if client:hexists(k, 'tabs') then
		local data = deserialize(client:hget(k, 'tabs')) -- get serialized table of objects
		fd.reduce(data, fd.map(setFruit), fd.into, ret) -- serialized objects
	    end
	    if client:hexists(k, 'msgs') then
		ret[#ret+1] = setFruit(deserialize(client:hget(k, 'msgs'))) -- get serialized object & serialized
	    end
	end

	return ret -- table of serialized objects

    -- store, short-circuit & re-route the message
    elseif cmd == 'msgs' then
	client:hset(k, cmd, s) -- store serialized object
	if ft then -- is there a session already opened on any client?
	    return { addFruit(ft)(w) }
	end

    elseif cmd == 'tabs' then
	local uuid = asUUID(client, s);
	if uuid then
	    client:hset(k, cmd, client:hget(IDS, uuid)) -- store serialized table of objects
	    client:hdel(IDS, uuid)
	    print('\n\tTabs data successfully stored\n')
	    client:hdel(AP, pid, ft)
	end

    elseif cmd == 'delete' then
	client:hdel(k, 'tabs')

    end

end

return tabs
