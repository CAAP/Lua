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

