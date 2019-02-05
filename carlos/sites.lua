-- module setup
local M = {}

-- Import Section

local fd = require'carlos.fold'

local concat = table.concat
local format = string.format
local remove = table.remove

local pairs = pairs
local type = type
local tonumber = tonumber
local tointeger = math.tointeger
local iolines = io.lines

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

local REGEX = "(%$[%u_]+)"

--------------------------------
-- Local function definitions --
--------------------------------

local function wrap(txt, tag) return format("<%s>%s</%s>", tag, txt, tag) end

local function json( w )
--    assert(type(w) == 'table')
    local ret = {}
    for k,v in pairs(w) do
	local u = type(v) == 'table' and json(v) or (tointeger(v) or v)
	ret[#ret+1] = format('%q: %'..(tonumber(u) and 's' or 'q'), k, u)
    end
    return format( '{%s}', concat(ret, ', ') ):gsub('"%[', '['):gsub(']"',']'):gsub("'", '"')
end

local function line_space(iter) fd.first(fd.wrap(iter), function(x) return x:match'^[%s]+$' end) end

local function get_lines(iter, boundary) return concat(fd.conditional(function(l) return l:match(boundary) end, fd.wrap(iter), fd.into, {}), '\n') end

--    vars[nm] = fd.first(fd.wrap(lines), function(x) return x:match'[^%s]+' end)

local function post_form( path )
    local vars = {}
    local lines = iolines( path )
    local boundary = fd.first(fd.wrap(lines), function(x) return x:match'Boundary' end)

    local nm, line
    repeat
	line = lines()
	nm = line:match'="([^=]+)"'
	line_space(lines)
	vars[nm] = get_lines(lines, boundary)
    until nm == 'file'

    return vars
end

---------------------------------
-- Public function definitions --
---------------------------------

M.REGEX = REGEX

M.wrap = wrap

M.json = json

M.post = post_form

function M.html()
    local MM = {}

    local scs = {after={}}
    local scs2 = scs.after
    local css = {}
    local body = {}
    local env = {}
    local head = ''
    local lang = 'es-MX'

    function MM.add_jscript(txt, after)
	if after then
	    scs2[#scs2+1] = txt
	else
	    scs[#scs+1] = txt
	end
	return MM
    end

    function MM.add_css(txt) css[#css+1] = txt; return MM end

    function MM.add_body(txt) body[#body+1] = txt; return MM end

    function MM.set_head(txt) head = txt; return MM end

    function MM.set_lang(txt) lang = txt; return MM end

    function MM.asstr()
	local ccc = #scs2 > 0 and format("(function() { let oldload = window.onload; window.onload = function() { oldload && oldload(); %s }; })();", concat(scs2, '\n')) or ''
	local ret = '<!DOCTYPE html><html lang="$LANG">$ENV</html>'
	env['$LANG'] = 'es-MX'
	env[1] = wrap(concat({head, wrap(concat({'"use strict";', ccc, concat(scs, '\n')}, '\n'), 'script'), wrap(concat(css, '\n'), 'style')}, '\n'), 'head')
	env[2] = wrap(concat(body, '\n'), 'body')
	env['$ENV'] = concat(env, '\n')
	return ret:gsub(REGEX, env)
    end

    return MM
end

return M
