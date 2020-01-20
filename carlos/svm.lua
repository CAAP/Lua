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
 *    [1, {2,0.1, 3,0.2, -1,0.0}], [2, {2,0.1, 3,0.3, 4,-1.2, -1,0.0}], ...
 *    [1, {1,0.4, -1,0.0}], [2 {2,0.1, 4,1.4, 5,0.5, -1,0.0}], ...
 *    [3, {1,-0.1, 2,-0.2, 3,0.1, 4,1.1, 5,0.1, -1,0.0}]
--]]


--[[
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


return M
