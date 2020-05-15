--module setup
local M = {}

-- Import section
local assert 	   = assert
local pairs 	   = pairs
local print 	   = print
local getmetatable = getmetatable

local castU16 	   = require'lints'.castU16
local castU32 	   = require'lints'.castU32

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

-- Local function for module-only access

local function socket(t, ppties, ctx)
    local skt = assert(ctx:socket(t:upper()))
    for pty,v in pairs(ppties) do
	local f = skt[pty]
	assert(f(skt,v))
    end
    return skt
end

---------------------------------
-- Public function definitions --
---------------------------------

function M.monitor(ctx, server, endpoint)
    assert( endpoint:match'inproc' )
    local skt = assert(ctx:socket'PAIR')
    assert( server:monitor( endpoint ) )
    assert( skt:connect( endpoint ) )

    local mt = getmetatable(skt)
    function mt:receive()
	local msgs = self:recv_msgs()
	assert(#msgs == 2) -- per specification
	assert(#msgs[1] == 6) -- per specification
	return self:monitor_event(castU16(msgs[1])), castU32(msgs[1]:sub(3,6)), msgs[2]
    end

    return skt
end


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


----------------------------------

return M

