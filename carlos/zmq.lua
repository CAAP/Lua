--module setup
local M = {}

-- Import section
local zmq = require'lzmq'
local fd = require'carlos.fold'
local assert = assert
local print = print

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

<<<<<<< HEAD
    function MM.send(id, data) return srv:send_msgs{id, data} end
=======
    function MM.send(id, s) return srv:send_msgs{id, s} end
>>>>>>> 2692a28183bceea11a51270d31420a0241604acc

    function MM.receive()
	local id, more = assert(srv:recv_msg())
	if more then more = fd.reduce(msgs, fd.into, {}) end
	return id, more
    end

    return MM
end

function M.socket(sktt, ctx)
    local ctx = ctx or assert(zmq.context())
    local skt = ctx:socket(sktt)

    local M = {endpoints={}}

    -- PUB,
    if sktt == 'PUB' or sktt == 'XPUB' then
    function M.bind(endpoint)
	local ends = M.endpoints
	local endpoint = endpoint or "tcp://*:5555"
	local p, err = assert(skt:bind(endpoint))
	if p then ends[#ends+1] = endpoint; return p end
	return p,err
    end

    function M.unbind(endpoint)
	local ends = M.endpoints
	assert(endpoint and ends[endpoint], "EROR: Endpoint not previously bound!")
	local p,err = assert(skt:unbind(endpoint))
	if p then ends[endpoint] = nil; return p end
	return p,err
    end

    -- SUB,
    else
    M.tags = {}

    function M.connect(endpoint)
	local ends = M.endpoints
	local endpoint = endpoint or "tcp://localhost:5556"
	local p,err = assert(skt:connect(endpoint))
	if p then ends[#ends+1] = endpoint; return p end
	return p,err
    end

    function M.disconnect(endpoint)
	local ends = M.endpoints
	assert(endpoint and ends[endpoint], "EROR: Endpoint not previously connected!")
	local p,err = assert(skt:disconnect(endpoint))
	if p then ends[endpoint] = nil; return p end
	return p,err
    end

    function M.subscribe(tag)
	local tags = M.tags
	local tag = tag or ""
	local p,err = assert(skt:subscribe(tag))
	if p then tags[#tags+1] = tag; return p end
	return p,err
    end

    function M.unsubscribe(tag)
	local tags = M.tags
	assert(tag and tags[tag], "EROR: Tag not previously subscribed!")
	local p,err = assert(skt:unsubscribe(tag))
	if p then tags[tag] = nil; return p end
	return p,err
    end
    end -- fi

    function M.ls() return fd.reduce(fd.keys(M), function(_,x) print(x) end) end

    return M
end

----------------------------------

return M

