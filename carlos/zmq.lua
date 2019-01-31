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

    function MM.send(...) return srv:send_msgs{...} end

    function MM.receive()
	local id, more = assert(srv:recv_msg())
	if more then more = fd.reduce(msgs, fd.into, {}) end
	return id, more
    end

    return MM
end

----------------------------------

return M
