-- module setup
local M = {}

-- Import Section

local concat = table.concat
local format = string.format

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

local REGEX = "(%$[%u_]+)"

--------------------------------
-- Local function definitions --
--------------------------------

local function read_file(path)
    local f = io.open(path)
    local s = f:read'a'
    f:close()
    return s
end

local function wrap(txt, tag) return format("<%s>%s</%s>", tag, txt, tag) end

---------------------------------
-- Public function definitions --
---------------------------------

M.wrap = wrap

function M.html()
    local MM = {}

    local scs = {after={}}
    local scs2 = scs.after
    local css = {}
    local body = {}
    local env = {}
    local title = ''
    local lang = 'es-MX'

    function MM.add_script(txt, after)
	if after then
	    scs2[#scs2+1] = txt
	else
	    scs[#scs+1] = txt
	end
	return MM
    end

    function MM.add_css(txt) css[#css+1] = txt; return MM end

    function MM.add_body(txt) body[#body+1] = txt; return MM end

    function MM.set_title(txt) title = txt; return MM end

    function MM.set_lang(txt) lang = txt; return MM end

    function MM.asstr()
	local ccc = #scs2 > 0 and format("(function() { let oldload = window.onload; window.onload = function() { oldload && oldload(); %s }; })();", concat(scs2, '\n')) or ''
	local ret = '<!DOCTYPE html><html lang="$LANG">$ENV</html>'
	env['$LANG'] = 'es-MX'
	env[1] = wrap(concat({title, wrap(concat({ccc, concat(scs, '\n')}, '\n'), 'script'), wrap(concat(css, '\n'), 'style')}, '\n'), 'head')
	env[2] = wrap(concat(body, '\n'), 'body')
	env['$ENV'] = concat(env, '\n')
	return ret:gsub(REGEX, env)
    end

    return MM
end

return M

