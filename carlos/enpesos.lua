-- module setup
local M = {}

-- Import Section
local int = math.tointeger
local tostring = tostring
local assert = assert
local type = type

local format = string.format
local concat = table.concat

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
local centenas = {'CIENTO', 'DOSCIENTOS', 'TRESCIENTOS', 'CUATROSCIENTOS', 'QUINIENTOS', 'SEISCIENTOS', 'SETECIENTOS', 'OCHOCIENTOS', 'NOVECIENTOS'}

local unidades = {'UN', 'DOS', 'TRES', 'CUATRO', 'CINCO', 'SEIS', 'SIETE', 'OCHO', 'NUEVE'}

local decenas = {'', 'VEINTI', 'TREINTA Y', 'CUARENTA Y', 'CINCUENTA Y', 'SESENTA Y', 'SETENTA Y', 'OCHENTA Y', 'NOVENTA Y', 'DIEZ', 'ONCE', 'DOCE', 'TRECE', 'CATORCE', 'QUINCE', 'DIECISEIS', 'DIECISIETE', 'DIECIOCHO', 'DIECINUEVE', 'VEINTE'}

local suffix = {}

--------------------------------
-- Local function definitions --
--------------------------------
local function enpesos(z)
    if type(z) == "number" then z = tostring(z) end
    local y,c = z:match'(%d+)%.(%d+)'
    local N = #y
    local ret = {}

    suffix[4] = 'MIL'; suffix[7] = 'MILLON'

    local function digit(i) return int(y:sub(i,i)) end

    if N == 1 and y == '1' then return format('UN PESO %s/100 M.N.', c) end
    if N == 1 and y == '0' or N == 0 then return format('ZERO PESOS %s/100 M.N.') end
    if N > 7 or (N==7 and digit(1) > 1) then suffix[7] = 'MILLONES' end

    y = y:reverse()

    while N > 0 do
	local M = N%3
	if M == 0 then ret[#ret+1] = int(y:sub(N-2, N):reverse()) == 100 and 'CIEN' or centenas[digit(N)] end
	if M == 1 then -- unidades
	    ret[#ret+1] = unidades[digit(N)]
	    if suffix[N] then ret[#ret+1] = suffix[N] end
	end
	if M == 2 then -- decenas
	    local x = int(y:sub(N-1, N):reverse())
	    if x>9 then
		local p = decenas[x] or ((x%10 == 0) and decenas[digit(N)]:gsub(' Y', ''))
		if p then ret[#ret+1] = p; N = N - 1 else ret[#ret+1] = decenas[digit(N)] end
		if suffix[N] then ret[#ret+1] = suffix[N] end
	    end
	end
	N = N - 1
    end

    ret[#ret+1] = format('PESOS %s/100 M.N.', c)

    return concat(ret, ' '):gsub('VEINTI ','VEINTI')
end

---------------------------------
-- Public function definitions --
---------------------------------
M.enpesos = enpesos

function M.test()
    assert(enpesos'100.50' == 'CIEN PESOS 50/100 M.N.', 100.50)
    assert(enpesos'116.45' == 'CIENTO DIECISEIS PESOS 45/100 M.N.', 116.45)
    assert(enpesos'218.64' == 'DOSCIENTOS DIECIOCHO PESOS 64/100 M.N.', 218.64)
    assert(enpesos'23.63' == 'VEINTITRES PESOS 63/100 M.N.', 23.63)
    assert(enpesos'2020.03' == 'DOS MIL VEINTE PESOS 03/100 M.N.', 2020.03)
    assert(enpesos'1040.21' == 'UN MIL CUARENTA PESOS 21/100 M.N.', 1040.21)
    assert(enpesos'2030153.12' == 'DOS MILLONES TREINTA MIL CIENTO CINCUENTA Y TRES PESOS 12/100 M.N.', 2030153.12)
    return true
end


return M
