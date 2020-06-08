-- module setup
local M = {}

-- Import Section
local fd = require'carlos.fold'
local letra = require'carlos.enpesos'.enpesos

local concat = table.concat
local remove = table.remove
local format = string.format
local floor = math.floor
local int = math.tointeger
local popen = io.popen

local print = print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
local width = 38
local forth = {5, 7, 4, 12, 10}

--------------------------------
-- Local function definitions --
--------------------------------
local function centrado(s)
    local m = s:len()
    local n = floor((width-m)/2 + 0.5) + m
    return format('%'..n..'s', s)
end

local function derecha(s) return format('%'..width..'s', s) end

local function campos(w)
    return concat(fd.reduce(w, fd.map(function(x,j) return format('%'..forth[j]..'s', x) end), fd.into, {}), '')
end

---------------------------------
-- Public function definitions --
---------------------------------
function M.ticket(head, data)
    local ret = {'\27\60', '',
	centrado'FERRETERIA AGUILAR',
	centrado'FERRETERIA Y REFACCIONES EN GENERAL',
	centrado'Benito Ju\225rez 1-C, Ocotl\225n, Oaxaca',
	centrado'Tel. (951) 57-10076',
	' ',
	' ',
	campos{'CLAVE', 'CNT', '%', 'PRECIO', 'TOTAL'},
	' '
    }

    local function procesar(w)
	local unidad = w.unidad or 'PZ' -- and w.unidad:match'[^_]+' 
	local unitario = w.unitario or 0
	ret[#ret+1] = w.desc
	ret[#ret+1] = campos{w.clave, w.qty, w.rea, unitario..' '..unidad, w.subTotal}
	if (w.uidSAT) then
	    local uid = int(w.uidSAT) or 0
	    uid = uid>0 and uid or 'XXXXX'
	    ret[#ret+1] = campos{uid, '', '', '', ''}
	end
    end

    local function finish(w)
	local fecha, hora = w.uid:match('([^T]+)T([^P]+)')
	ret[2] = centrado(w.tag:upper())
	ret[7] = centrado(format('Fecha: %s | Hora: %s', fecha, hora))
	ret[#ret+1] = ''
	ret[#ret+1] = derecha(w.total)

	if w.iva then
	    remove(ret, 3); remove(ret, 3); remove(ret, 3); remove(ret, 3); remove(ret, 3)
	    ret[#ret+1] = derecha(format('I.V.A.   %s', w.iva))
	    ret[#ret+1] = derecha(format('TOTAL    %s', w.ttotal))
	    return
        end

	local m = letra(w.total)
	if #m > width then ret[#ret+1] = m:sub(1, width); ret[#ret+1] = m:sub(width+1)
	else ret[#ret+1] = m end
	ret[#ret+1] = format('\n%s', w.nombre and w.nombre:upper() or '')
    end

    if #data > 0 then fd.reduce(data, function(w) procesar(w) end); finish(head) end

    ret[#ret+1] = centrado'GRACIAS POR SU COMPRA'
    ret[#ret+1] = '\27\100\7 \27\105'
    return ret
end


function M.bixolon(endpoint, s)
    local p = popen("nc ".. endpoint,'w')
    local ret = p:write(s)
    p:close()
    return ret
end

return M

