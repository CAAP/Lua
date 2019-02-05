#! /usr/bin/env lua53

local zmq = require'lzmq'
local fd = require'carlos.fold'

local ctx = assert(zmq.context())
local front = assert(ctx:socket'SUB')
local back  = assert(ctx:socket'XPUB')

assert(front:connect'tcp://localhost:5557')
assert(back:bind'tcp://*:5558')
front:subscribe'' -- subscribe to every topic

local cache = {}
local poll = assert(zmq.pollin{front, back})

local j = 1

while true do
    local i = assert(poll(1000)) -- 1 second
    if i == -1 then break end -- interrupted
    if i == 1 then -- front, cache & forward data
	local m = assert(front:recv_msg())
	assert(#m > 0, "EROR: empty message received!")
	if j%100 == 0 then print('Front:', m, '\n') end
	local k = m:match'%d+'
	if not k then
	    print(m, '\n')
	    print'Ooops!\n'
	    break
	end
	cache[k] = m
	assert(back:send_msg(m))
	j = j+1
    elseif i == 2 then -- back, new sub, pull data
	local subs = assert(back:recv_msg()) -- subscription event
	if subs:byte() == 1 then
	    local k = subs:sub(2,-1):match'%d+'
	    print('Subscription:', k, '\n')
	    if cache[k] then assert(back:send_msg(cache[k])) end
	end
    end
end

