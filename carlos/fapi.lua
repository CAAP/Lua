local M = {}

-- Import Section
--
local fd	  = require'carlos.fold'

local asJSON	  = require'carlos.json'.asJSON

local exec	  = os.execute
local remove	  = table.remove
local concat	  = table.concat
local format	  = string.format
local pairs	  = pairs
local assert	  = assert
local print	  = print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--
local HOST = 'https://www.facturapi.io/v1/customers'

local KEYS = '"sk_test_YJj1GZ6r5bvWNKxzDR11klaMo7LeXQE4:R5Cwje3MZLZzvDA"'

-- tax_id = RFC; neighborhood = colonia; municipality = municipio o delegacion
local CLIENTE = {"legal_name", "tax_id", "email"}

CLIENTE.opcional = {"phone", "address", "address.street", "address.exterior", "address.interior", "address.neighborhood", "address.zip", "address.city", "address.municipality", "address.state"}

local UPDATE  = {"id"}

local QUERY   = {"id"}

--------------------------------
-- Local function definitions --
--------------------------------
--
local function post(req)
    local opts = { 'curl', HOST }
    for k,v in pairs(req) do opts[#opts+1] = format("-%s %s", k, v) end
    return concat(opts, ' ')
end

local function missing(a, fields)
    return fd.first(fields, function(k) return not(a[k]) end)
end

local function quote(s)
    local pred = s:match'"'
    return format(pred and "'%s'" or '%q', s)
end

---------------------------------
--	   Public API	       --
---------------------------------
--

function M.newCustomer(data, path)
    assert(not(missing(data, CLIENTE)), "Missing field!\n")
    local ret = { d=quote(asJSON(data)), H='"Content-Type: application/json"', u=KEYS, s='', o=quote(path) }
    ret = post( ret )
    print( ret, '\n' )
    return exec( ret )
end

return M
