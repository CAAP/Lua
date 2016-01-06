-- module setup
local M = {}

-- Import Section
local pow=math.pow
local exp=math.exp

-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

function M.gaussian(x,s,w)
    local h = s or 1
    local a = w or 1

    return a*exp(-pow((x/h), 2))
end

function M.sigmoid(x,w)
   local h = w or 1

    return 2/(1+exp(x/h))
end

function M.logistic(x, w)
    local h = w or 1

    return 4/(exp(x/h) + 2 + exp(-x/h))
end

function M.exponential(x, alfa)
    local a = alfa or 1

    return exp(-a*x)
end

return M
