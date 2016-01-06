-- module setup
local M = {}

-- Import Section
sts=require'lcdf'
local dt = sts.dstud -- x & df
local pt = sts.pstud -- t & df; cumulative distribution
local qt = sts.qstud -- p & df; cumulative distribution

local dn = sts.dnorm -- x [, mean, sd]
local pn = sts.pnorm -- x [, mean, sd]; cumulative distribution
local qn = sts.qnorm -- p [, mean, sd]; cumulative distribution

local dc = sts.dchisq -- x & df [, nonc ]
local pc = sts.pchisq -- x & df [, nonc ]; cumulative distribution
local qc = sts.qchisq -- p & df [, nonc ]; cumulative distribution

local df = sts.dfstats -- x, dfn & dfd
local pf = sts.pfstats -- x, dfn & dfd [, nonc ]
local qf = sts.qfstats -- p, dfn & dfd [, nonc ]

local pow = math.pow
local sqrt = math.sqrt
local abs = math.abs
local unpack = table.unpack
local ln = math.log

local tr = require'carlos.transducers'

-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions
-- sig2 := sigma_squared (aka variance)
-- df := degrees of freedom

-- INPUT
-- n : sample size
-- s2 : sigma_squared, variance
-- avg : mean, average

local function pooledvar(samples)
    local ret = tr.reduce( samples, function(a, x) a.k=a.k+1; a.n=a.n+x.n; a.sum=a.sum+(x.n-1)*x.s2; a.lsum=a.lsum+(x.n-1)*ln(x.s2) end, {sum=0, lsum=0, n=0, k=0} )
    ret.s2 = ret.sum/(ret.n-ret.k)
    return ret
end

function M.chisq(a)
    return pc(a.chisq, a.df)
end

function M.bartlett(samples)
    local sig = pooledvar(samples)
    local factor = tr.reduce( samples, function(a, x) a.sum=a.sum+1/(x.n-1) end, {sum=0} ).sum
    local delta = sig.n - sig.k
    local chisq = (delta*ln(sig.s2) - sig.lsum) / (1 + (factor-1/delta)/(3*(sig.k-1)))
    return chisq, qc(0.95, sig.k-1), 1-pc(chisq, sig.k-1)
end

-- one-sample
function M.one(a)
    local sig2 = a.s2/a.n
    local df = a.n - 1

    return a.avg, sig2, df
end

-- pooled: unequal sample sizes & equal variance
function M.pooled(a, b)
    local sig2 = ((a.n-1)*a.s2 + (b.n-1)*b.s2) / (a.n + b.n - 2)
    local factor = 1/a.n + 1/b.n
    local df = a.n + b.n - 2
    return abs(a.avg-b.avg), sig2*factor, df
end

-- unequal variances
function M.unequal(a, b)
    local s2a = a.s2/a.n
    local s2b = b.s2/b.n
    local sig2 = s2a + s2b
    local df = pow(sig2, 2) / ( pow(s2a,2)/(a.n-1) + pow(s2b,2)/(b.n-1) )
    return abs(a.avg - b.avg), sig2, df
end

-- one-tail Student's t-test
function M.ttest(delta, sig2, df)
    local tstat = delta/sqrt(sig2)
    local p = 1 - pt(tstat, df)

    return {p=p, t=tstat}
end

-- interquartile range
function M.iqr(ks, k)
    local kk = k or 1.5
    local iqr = kk*(ks[3]-ks[1])
    return ks[1]-iqr, iqr+ks[3]
end

return M
