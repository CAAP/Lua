
local zmq = require'lzmq'

local TIMEOUT  = 2500 -- msec
local RETRIES  = 3
local ENDPOINT = "tcp://localhost:5555"

local ctx = assert(zmq.context())
print'I: connecting to server...\n'
local client = assert(ctx:socket'REQ')
assert(client:connect(ENDPOINT))

local j = 0
local retries_left = RETRIES

while retries_left > 0 do
    client:send_msg(string.format('%d', j))



end


