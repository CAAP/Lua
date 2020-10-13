
-- Import Section
--
local asUUID	= require'carlos.ferre.uuids'
local rconnect	= require'redis'.connect

local format	= string.format
local insert	= table.insert
local concat	= table.concat
local assert	= assert
local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local client	= assert( rconnect('127.0.0.1', '6379') )

local AP	= 'app:active'
local MM	= 'app:mem:'
local IDS	= 'app:uuids:'

--------------------------------
-- Local function definitions --
--------------------------------
--
local function tabs(cmd, msg)
    local pid = msg[2]:match'pid=(%d+)' or msg[1]:match'pid=(%d+)'
    local ft = client:hget(AP, pid)

    if cmd == 'login' then
	local fruit = msg[2]:match'fruit=(%a+)'
	local ret = {}
	-- guard against double login by sending message to first session & closing it
	if ft and ft ~= fruit then
	    ret[1] = format('%s logout pid=%d', ft, pid)
	    client:hdel(AP, ft)
	end
	-- in any case
	client:hset(AP, fruit, pid, pid, fruit) -- new session open
	print('\n\tSuccessful login for', fruit, '\n')
	local ID = MM..pid
	if client:hexists(ID, 'msgs') then ret[#ret+1] = client:hget(ID, 'msgs'):gsub('$FRUIT', fruit) end
	if client:hexists(ID, 'tabs') then ret[#ret+1] = client:hget(ID, 'tabs'):gsub('$FRUIT', fruit) end
	return ret -- returns a table | possibly empty

    -- store, short-circuit & re-route the message
    elseif cmd == 'msgs' then
	insert(msg, 1, '$FRUIT') -- placeholder for FRUIT
	insert(msg, '\n\n')
	client:hset(MM..pid, 'msgs', concat(msg, ' ')) -- store msg
	if client:hexists(AP, pid) then -- is session open in any client?
	    return { client:hget(MM..pid, 'msgs'):gsub('$FRUIT', client:hget(AP, pid)) }
	end

    elseif cmd == 'tabs' then
	local uuid = asUUID(client, cmd, msg[2])
	if uuid then
	    local msg = format('$FRUIT tabs pid=%d&%s\n\n', pid, client:hget(IDS..uuid, 'data'))
--	    client:del(IDS..uuid) -- XXX expires after 60 secs
	    client:hset(MM..pid, 'tabs', msg)
	    print('\n\tTabs data successfully stored\n')
	    client:hdel(AP, pid, ft)
	end

    elseif cmd == 'delete' then
	client:hdel(MM..pid, 'tabs')

    else
	print('\nERROR: Unknown command', cmd,'\n')

    end
end

return tabs
