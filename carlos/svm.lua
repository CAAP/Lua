-- module setup
local M = {}

-- Import Section
local fd = require'carlos.fold'
local asint = math.tointeger

local nodes = require'lsvm'.nodes


-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

local function cmatrix( ys, preds )
    local tp, tn, fp, fn = 0, 0, 0, 0

    for i,y in ipairs(ys) do
	if y == preds[i] then
	    if y > 0 then tp = tp+1 else tn = tn+1 end
	else
	    if y > 0 then fn = fn+1 else fp = fp+1 end
	end
    end

    return {tp=tp, tn=tn, fp=fp, fn=fn}
end

local function accuracy( cmat )
    cmat.sn = cmat.tp / (cmat.tp + cmat.fn)
    cmat.sp = cmat.tp / (cmat.tp + cmat.fp)
    cmat.nsn = cmat.tn / (cmat.tn + cmat.fp)
    cmat.nsp = cmat.tn / (cmat.tn + cmat.fn)

    return cmat
end

--    svm_data of the form
--    <label> <index>:<value> <index>:<value> ...
--    e.g. +1 1:0.708333 2:1 3:1 4:-0.320755 5:-0.105023 6:-1 ...

local function svmline(line)
    local label = asint( line:match'[%+%-]%d' )
    local ret = {label}
    for i,v in line:gmatch'(%d+):([%.%-%d]+)' do
	ret[#ret+1] = i
	ret[#ret+1] = v
    end
    return ret
end

--[[
--	The Cascade SVM
--  Parallel Support Vector Machines
--  Graf, Cosatto, Bottou, Durdanovic, Vapnik
--
--  The core of an SVM is a quadratic programming
--  problem, separating SVs from the rest.
--  QP solvers become impractically slow for
--  problems sizes on the order of 100,000 points.
--
--  Our approach for accelerating the QP is based
--  on 'chunking', where subsets of the data are
--  optimized iteratively.
--
--  A distributed architecture, smaller optimizations
--  are solved independently and can be spread over
--  multiple processors, yet the ensamble is guaranteed
--  to converge to the globally optimal solution.
--]]





--[[
 *  EXAMPLE
 *
 *  LABEL  ATTR1 ATTR2 ATTR3 ATTR4 ATTR5
 *    1      0    0.1   0.2    0     0
 *    2      0    0.1   0.3  -1.2    0
 *    1     0.4    0     0     0     0
 *    2      0    0.1    0    1.4   0.5
 *    3    -0.1  -0.2   0.1   1.1   0.1
 *
 *  *** SPARSE - LUA ***
 *    1      2, 0.1   3, 0.2
 *    2      2, 0.1   3, 0.3   4, -1.2
 *    1      1, 0.4
 *    2      2, 0.1   4, 1.4   5, 0.5
 *    3      1, -0.1  2, -0.2  3, 0.1   4, 1.1   5, 0.1
 *
 *  DATA - C
 *    {1, 2, 0.1, 3, 0.2, -1, 0.0}, {2, 2, 0.1, 3, 0.3, 4, -1.2, -1, 0.0},
 *    {1, 1, 0.4, -1, 0.0}, {2, 2, 0.1, 4, 1.4, 5, 0.5, -1, 0.0},
 *    {3, 1,-0.1, 2,-0.2, 3,0.1, 4,1.1, 5,0.1, -1,0.0}

--    svm_data of the form
--    <label> <index>:<value> <index>:<value> ...
--    e.g. +1 1:0.708333 2:1 3:1 4:-0.320755 5:-0.105023 6:-1 ...
--    is converted to data of the form
--    {label, index1, value1, index2, value2, index3, value3, ...}
--    e.g. {1, 1, 0.708333, 2, 1, 3, 1, 4, -0.320755, ...}
--]]
function M.readsvm( data )
    local ret = fd.reduce(data, fd.map(svmline), fd.into, {})
    local svmNodes = nodes(ret)
    local ys = fd.reduce(ret, fd.map(function(a) return a[1] end), fd.into, {})

    return ret, ys, svmNodes
end


--[[
 *  EXAMPLE
 *
 *  training instance xi:
 *  <label> 0:i 1:K(xi,x1) ... L:K(xi,xL)
 *
 *  the first column must be the ID of xi
 *
 *  testing instance x:
 *  <label> 0:? 1:K(x,x1) ... L:K(x,xL)
 *
 *  the first column can be any value ?
 *
--]]
function M.precomputed( data )
end

return M
