--module setup
local M = {}

-- Import section
local zmq = require'lzmq'
local fd = require'carlos.fold'
local assert = assert

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

-- Local function for module-only access


---------------------------------
-- Public function definitions --
---------------------------------

function M.stream(endpoint)
    local ctx = assert(zmq.context())
    local srv = assert(ctx:socket'STREAM')
    assert(srv:bind(endpoint))
    local MM = {}

    local function msgs() return srv:recv_msgs() end -- returns iter, state & counter

    function MM.close(id) assert(srv:send_msgs{id, ""}) end

    function MM.send(id, s) return srv:send_msgs{id, s} end

    function MM.receive()
	local id, more = assert(srv:recv_msg())
	if more then more = fd.reduce(msgs, fd.into, {}) end
	return id, more
    end

    return MM
end

function M.cache(frontend, backend)
    local ctx = assert(zmq.context())
    local front = assert(ctx:socket'SUB')
    local back  = assert(ctx:socket'XPUB')

    assert(front:connect(frontend or 'tcp://localhost:5557'))
    assert(back:bind(backend or 'tcp://*:5558'))
    assert(front:subscribe'')

    local poll = assert(zmq.pollin{front, back})

end

----------------------------------

return M

