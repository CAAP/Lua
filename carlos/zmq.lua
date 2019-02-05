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
-- A socket of type ZMQ_STREAM is used to send and receive TCP
-- data from a non-ZMQ peer, when using the tcp:// transport.
-- It can act as a client and/or server, sending and/or receiving
-- TCP data asynchronously.
-- When RECEIVING data, it shall prepend a message part containing
-- the identity of the originating peer to the message before
-- passing it to the application. Messages received are fair-queued
-- from among all connected peers.
-- When SENDING data, it shall remove the first part of the message
-- and use it to determine the identity of the peer the message
-- shall be routed to.
-- To OPEN a connection to a server, use the zmq_connect call, and
-- fetch the socket identity (ZMQ_IDENTITY).
-- To CLOSE a specific connection, send the identity frame followed
-- by a zero-length message.
-- When a CONNECTION is made, a zero-length message will be received
-- by the application.
-- Similarly, when a peer DISCONNECTS (or the connection is lost), a
-- zero-length message will be received by the application.
-- You MUST send one identity frame followed by one data frame. The
-- ZMQ_SNDMORE flag is required for identity frames byt is ignored
-- on data frames.
function M.stream(endpoint)
    local ctx = assert(zmq.context())
    local srv = assert(ctx:socket'STREAM')
    assert(srv:bind(endpoint))
    local MM = {}

    local function msgs() return srv:recv_msgs() end -- returns iter, state & counter

    function MM.close(id) assert(srv:send_msgs{id, ""}) end

    function MM.send(id, data) return srv:send_msgs{id, data} end

    function MM.receive()
	local id, more = assert(srv:recv_msg())
	if more then more = fd.reduce(msgs, fd.into, {}) end
	return id, more
    end

    return MM
end

----------------------------------

return M

