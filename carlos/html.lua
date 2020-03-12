-- module setup
local M = {}

-- Import Section
local format = require'string'.format
local char = require'string'.char

local concat = table.concat
local tonumber = tonumber
local insert = table.insert

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
local post = 'POST %s HTTP/1.1\r\nHost: %s\r'
local protocol = 'HTTP/1.1 %s\r'
local access = 'Access-Control-Allow-Origin: %s\r\nAccess-Control-Allow-Methods: GET\r\n\r'
local auth = 'Authorization: %s\r'
local content = { text='Content-Type: text/plain\r\nCache-Control: no-cache\r',
 -- Content-Type: text/plain\r
		  soap='Content-Type: application/soap+xml; charset=utf-8\r',
		  xml='Content-Type: text/xml; charset=utf-8\r',
		  urlencoded='Content-Type: application/x-www-form-urlencoded\r',
		  json='Content-Type: application/json\r\nConnection: keep-alive\r\nAccept: */*\r',
		  post='Accept: */*\r\nContent-Type: application/json\r',
		  stream='Content-Type: text/event-stream\r\nConnection: keep-alive\r\nCache-Control: no-cache\r' }

local status = {ok='200 OK'}

--------------------------------
-- Local function definitions --
--------------------------------

local function hex(h) return char(tonumber(h,16)) end

local function urldecode(s) return s:gsub("+", "|"):gsub("%%(%x%x)", hex) end

---------------------------------
-- Public function definitions --
---------------------------------

-- w | status='ok', content='text', ip='*', [body]
function M.response(w)
    local ret = { format(protocol, status[w.status] or status.ok),
		  content[w.content] or content.text,
		  format(access, w.ip or '*') }
    if w.body then ret[#ret+1] = w.body end
    ret[#ret+1] = '\n'
    return concat(ret, '\n')
end

function M.post(w)
    local k = w.url:find'/'
    local ret = { format(post, w.url:sub(k), w.url:sub(1, k-1)),
		  content[w.content] or content.post,
		  format('Content-Length: %d\r\n\r', #w.body) }
    if w.action then ret[#ret+1] = ret[#ret]; ret[#ret-1] = format('SOAPAction: %q\r', w.action) end
    ret[#ret+1] = w.body
    if w.auth then insert(ret, 2, format(auth, w.auth)) end
    return concat(ret, '\n')
end

return M
