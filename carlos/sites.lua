-- module setup
local M = {}

-- Import Section

local concat = table.concat

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access


--------------------------------
-- Local function definitions --
--------------------------------

local function read_file(path)
    local f = io.open(path)
    local s = f:read'a'
    f:close()
    return s
end

local function wrap(txt, tag) return string.format("<%s>%s</%s>", tag, txt, tag) end

---------------------------------
-- Public function definitions --
---------------------------------

M.wrap = wrap

function M.html()
    local MM = {}

    local ret = [==[
<!doctype html>
<html lang="$LANG">
  $ENV
</html>
]==]

    local scs = {}
    local css = {}
    local body = {}
    local env = {}
    local title = ''

    local function add_script(txt) scs[#scs+1] = txt; return MM end

    local function add_css(txt) css[#css+1] = txt; return MM end

    local function add_body(txt) body[#body+1] = txt; return MM end

    local function set_title(txt) title = txt; return MM end

    local function asstr()
	env['$LANG'] = 'es_MX'
	env[1] = wrap(concat({title, wrap(concat(scs, '\n'), 'script'), wrap(concat(css, '\n'), 'style')}, '\n'), 'head')
	env[2] = wrap(concat(body, '\n'), 'body')
	env['$ENV'] = concat(env, '\n')
	return ret:gsub(env)
    end

    return MM
end

return M

