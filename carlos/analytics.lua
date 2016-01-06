-- module setup
local M = {}

-- Import Section
local sort=table.sort
local print=print
local assert=assert

local tr = require'carlos.transducers'

-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Local function definitions
local function byQuantiles(data, idx, n)
    local xs = tr.map( data, function(row) return row[idx] end ); sort(xs)
    local q = 1/n
    local ks = ops.range( q, 1, q)
    ks = tr.map( ks, function(k) return ops.quantile(xs, k) end)
    local function rank( row ) return ops.bsearch(#ks, function(i) return ks[i] < row[idx] end)  end
    local ret = tr.map( ks, function() return oo.new( data ) end )
    ret.lut = ks; ret.idx = #ks; ret.keys = ops.range(1, #ks, 1)
    tr.groupby( data, rank, ret )
    return ret
end

local function append( a,v )
    local ret = tr.map( a, function(x) return x end )
    ret[#ret+1] = v
    return ret
end

-- Function definitions
function M.tree()
    local MM, keys = {}, {}
    local function rollup(x) return #x end

    local function nest( arr, k )
        if k > 0 then
            local ret = tr.groupby( arr, keys[k] )
	    ret.depth = k
            for i=1, #ret do
		if k == 1 then ret[i] = rollup( ret[i] )
		else ret[i] = nest( ret[i], k-1 ) end
	    end
	    return ret
        else return arr end
    end

    function MM.key( f ) keys[#keys+1] = f; return MM end

    function MM.rollup( f ) rollup = f; return MM end

    function MM.flat( data, acc )
        assert( #keys > 0, 'No keys found!' )
	local lines = acc or {}
	local function flatten( arr, k )
            if k < #keys + 1 then
	        local ret = tr.groupby( arr, keys[k] )
		for i=1, #ret do
		    local ans = append( arr.acc or {}, ret.lut[i] )
		    if k == #keys then ans[#ans+1] = rollup( ret[i] ); lines[#lines+1] = ans
		    else ret[i].acc = ans; flatten( ret[i], k+1 ) end
		end
	    end
	end
	flatten( data, 1 )
	return lines
    end

    return MM
end

return M
