-- module setup
local M = {}

-- Import Section
local fd = require'carlos.fold'

local nodes = require'lsvm'.nodes


-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

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



return M
