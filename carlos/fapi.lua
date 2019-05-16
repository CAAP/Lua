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
local HOST	 = 'https://www.facturapi.io/v1'

local KEYS	 = '"sk_test_YJj1GZ6r5bvWNKxzDR11klaMo7LeXQE4:R5Cwje3MZLZzvDA"'

-- tax_id = RFC; neighborhood = colonia; municipality = municipio o delegacion
local CLIENTE	 = {"legal_name", "tax_id", "email"}

CLIENTE.opcional = {"phone", "address", "address.street", "address.exterior", "address.interior", "address.neighborhood", "address.zip", "address.city", "address.municipality", "address.state"}

local INGRESO	 = {"customer", "items", "payment_form"}

INGRESO.opcional = {"payment_method", "use", "folio_number", "series", "currency", "addenda"}

local ITEMS	 = {"product"}

ITEMS.opcional	 = {"quantity", "discount", "complement"}

local PRODUCT	 = {"description", "product_key", "price"}

PRODUCT.opcional = {"tax_included", "taxes", "unit_key", "unit_name", "sku"} -- sku : clave interna

local TAXES	 = {"rate", "type", "factor"} -- OPCIONAL

local UPDATE	 = {"id"}

local QUERY	 = {"id"}

local FPAGO	 = {['01']='Efectivo', ['02']='Cheque nominativo', ['03']='Transferencia electr&oacute;nica de fondos', ['04']='Tarjeta de cr&eacute;dito', ['28']='Tarjeta de d&eacute;bito', ['99']='Por definir' }

local MPAGO	 = {PUE='Pago en una sola exhibici&oacute;n (de contado)', PPD='Pago en parcialidades o diferido (a cr&eacute;dito)'}

local CFDI	 = {['G01']='Adquisici&oacute;n de mercancias', ['G02']='Devoluciones, descuentos o bonificaciones', ['G03']='Gastos en general'}

--------------------------------
-- Local function definitions --
--------------------------------
--
local function post(req, method)
    local opts = { 'curl', HOST .. method }
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

--[[
--	products
-- description, product_key|SATUID, price
-- tax_included, taxes, sku|clave|uid
--]]
local function products(items)
    assert(fd.first(items, function(it) return not(missing(it.product, PRODUCT)) end), "Missing field!\n")
    fd.reduce(items, function(it) it.product = asJSON(it.product) end)
    return items
end

--[[
--	items
-- product
-- quantity, discount
--]]
local function items(data)
    local its = data.items
    assert(fd.first(its, function(it) return not(missing(it, ITEMS)) end), "Missing field!\n")

    products(its)

    local ret = fd.reduce(its, fd.map(function(it) return asJSON(it) end), fd.into, {})
    data.items = format('[%s]', concat(ret, ',\n'))

    return data
end

---------------------------------
--	   Public API	       --
---------------------------------
--

--[[
--	customers
-- legal_name, tax_id|rfc, email
-- telefono, direccion { street, exterior, interior, zip, neighborhood|colonia city, municipality, state }
--]]
function M.newCustomer(data, rfc)
    assert(not(missing(data, CLIENTE)), rfc .. ": Missing field!\n")
    local ret = { d=quote(asJSON(data)), H='"Content-Type: application/json"', u=KEYS, s='', o=quote(rfc..'.txt') }
    ret = post( ret, '/customers' )
    print( ret, '\n' )
    return exec( ret )
end

function M.searchCustomer(query, path)
    local ret = { u=KEYS, s='', o=quote(path) }
    if query then
	ret.d = quote(format('{"q": %q}', query))
	ret.H = '"Content-Type: application/json"'
    end
    ret = post( ret, '/customers' )
    print( ret, '\n' )
--    return exec( ret )
end

--[[
--	invoices
-- customer, items, payment_form
-- payment_method, folio_number, series
--]]

function M.ingreso(data, path)
    assert(not(missing(data, INGRESO)), "Missing field!\n")

    items(data) -- make sure all fields are present

    local ret = { d=quote(asJSON(data)), H='"Content-Type: application/json"', u=KEYS, s='', o=quote(path) }
    ret = post( ret, '/invoices' )
    print( ret, '\n' )
--    return exec( ret )
end

return M
