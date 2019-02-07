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

--    function MM.send(id, data) return srv:send_msgs{id, data} end
    function MM.send(id, s) return srv:send_msgs{id, s} end

    function MM.receive()
	local id, more = assert(srv:recv_msg())
	if more then more = fd.reduce(msgs, fd.into, {}) end
	return id, more
    end

    return MM
end

-------------------------------------------------------
-- A socket of type REP is used by a service to receive requests from and send
-- replies to a client.
-- This socket type allows ONLY an alternating sequence of zmq_recv and
-- subsequent zmq_send calls. Each request received is fair-queued from among
-- all clients, and each reply sent is routed to the client that issued the last
-- request.
-- Compatible peer sockets: REQ & DEALER.
-------------------------------------------------------
-- A socket of type REQ is used by a client to send requests to and receive
-- replies from a service.
-- This socket type allows ONLY an alternating sequence of zmq_send and
-- subsequent zmq_recv calls. Each request sent is round-robined among all
-- services, and each reply received is matched with the last issued request.
-- Compatible peer sockets: REP & ROUTER.
-------------------------------------------------------
-- A socket of type DEALER is an advanced socket. Each message sent is
-- round-robined among all connected peers, and each message received is
-- fair-queued from all connected peers.
-- When a DEALER socket is connected to a REP socket each message sent must
-- consist of an empty message part, the delimiter, followed by one or more
-- body parts.
-------------------------------------------------------
-- A socket of type ROUTER is an advanced socket. When receiving a message,
-- a ROUTER socket shall prepend a message part containing the identity of the
-- originating peer to the message before passing it to the application.
-- Messages received are fair-queued from among all connected peers.
-- When sending messages a ROUTER socket shall remove the first part of
-- the message and use it to determine the identity of the peer the message
-- shall be routed to.
-- When a REQ socket is connected to a ROUTER socket, in addition to the
-- identity of the originating peer each message received shall contain an
-- empty delimiter message part. The entire structure of each received message
-- becomes: one or more identity parts, delimiter part, one or more body parts.
-- When sending replies to a REQ socket the application must include the
-- delimiter part.
-------------------------------------------------------

function M.socket(sktt, ctx)
    local ctx = ctx or assert(zmq.context())
    local skt = assert(ctx:socket(sktt))

    local MM = {endpoints={}}

    function MM.ls() return fd.reduce(fd.keys(MM), function(_,x) print(x) end) end

    -- PUB, XPUB, REP, ROUTER
    if sktt == 'PUB' or sktt == 'XPUB' or sktt = 'REP' or sktt = 'ROUTER' then
    function MM.bind(endpoint)
	local ends = MM.endpoints
	local endpoint = endpoint or "tcp://*:5555"
	local p, err = assert(skt:bind(endpoint))
	if p then ends[#ends+1] = endpoint; return p end
	return p,err
    end

    function MM.unbind(endpoint)
	local ends = MiM.endpoints
	assert(endpoint and ends[endpoint], "ERROR: Endpoint not previously bound!")
	local p,err = assert(skt:unbind(endpoint))
	if p then ends[endpoint] = nil; return p end
	return p,err
    end

    function MM.send( msg ) assert( skt:send_msg(msg) ) end

    -- REQ, DEALER, SUB, XSUB
    else

    function MM.connect(endpoint)
	local ends = MM.endpoints
	local endpoint = endpoint or "tcp://localhost:5556"
	local p,err = assert(skt:connect(endpoint))
	if p then ends[#ends+1] = endpoint; return p end
	return p,err
    end

    function MM.disconnect(endpoint)
	local ends = MM.endpoints
	assert(endpoint and ends[endpoint], "ERROR: Endpoint not previously connected!")
	local p,err = assert(skt:disconnect(endpoint))
	if p then ends[endpoint] = nil; return p end
	return p,err
    end

	if sktt == 'SUB' or sktt = 'XSUB' then
    MM.tags = {}
    function MM.subscribe(tag)
	local tags = MM.tags
	local tag = tag or ""
	local p,err = assert(skt:subscribe(tag))
	if p then tags[#tags+1] = tag; return p end
	return p,err
    end

    function MM.unsubscribe(tag)
	local tags = MM.tags
	assert(tag and tags[tag], "ERROR: Tag not previously subscribed!")
	local p,err = assert(skt:unsubscribe(tag))
	if p then tags[tag] = nil; return p end
	return p,err
    end
    	end
    end -- fi

    return MM
end

----------------------------------

return M

