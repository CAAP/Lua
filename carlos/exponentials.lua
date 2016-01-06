-- module setup
local M = {}

-- Import Section
local math=math
local assert=assert
local print=print
local tonumber=tonumber
local format=string.format

local ops = require'carlos.operations'
local tables = require'carlos.tables'
local tr = require'carlos.transducers'
local oo = require'carlos.oo'
--local lcdf = require'lcdf'
--local choose1 = lcdf.choose // WRONG!!!
local choose = ops.choose
local fact = ops.factorial

-- Local Variables for module-only access

local params = {}

function params:step( x, p )
    tr.reduce( self.t(x), function(a, y, i) a[i] = a[i] + y * p end, self )
end

function params:div( p )
    return tr.map( self, function(x) return x/p end )
end

-- Local function definitions

local function boxMuller( mu, sigma )
    local z = math.sqrt( - 2 * math.log(math.random()) ) * math.cos( 2 * math.pi * math.random() )
    return z * sigma + mu
end

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

-- Frank Nielsen and Vincent Garcia, 2011. Statistical exponential families: A digest with flash cards.

-- F: log normalizer (in natural parameters)
-- G: Legendre dual of the log normalizer := F*
-- t: sufficient statistics
-- k: carrier measure
-- theta2lambda: natual to source parameters
-- lambda2eta: source to expectation parameters
-- eta2lambda: expectation to source parameters

---------------------------------------
-- univariate Gaussian distribution  --
-- source (lambda) parameters: mu, sigma-sqr
-- natural parameters: theta_1, theta_2
-- expectation parameters: eta_1, eta_2
-- t: x, x^2 (expectation parameters)
---------------------------------------
function M.univariate()

    local MM  = {}

    local function G( eta ) return -0.5*math.log(math.abs(eta[1]*eta[1] - eta[2])) end

    local function gradG( eta )
        local q = eta[1]*eta[1]-eta[2]
        return { -eta[1]/q, 0.5/q }
    end

    local function t( x ) return { x, x*x } end

    local function KLD( LP, LQ )
        return 0.5*( 2*math.log( math.sqrt( LQ[1]/LP[2] ) ) + LP[2]/LQ[2] + (LQ[1]-LP[1])*(LQ[1]-LP[1])/LQ[2] - 1 )
    end

    function MM.eta2lambda( eta ) return { eta[1], eta[2] - eta[1]*eta[1]} end

    function MM.asstr( eta ) return format('%.2f, %.2f', eta[1], eta[2]) end

    function MM.density( x, lambda )
        return math.exp( -(x-lambda[1])*(x-lambda[1])/(2*lambda[2]) ) / math.sqrt( 2*math.pi*lambda[2] )
    end

    function MM.params()
	local ret = {0, 0}
	ret.t = t
	return oo.new( params, ret )
    end

--[[    function MM.moments( xs )
	local ret = {0, 0, 0}
	tr.reduce( xs, function(a, x) local w = t(x); a[1] = a[1] + w[1]; a[2] = a[2] + w[2]; a[3] = a[3] + 1 end, ret )
	return { ret[1]/ret[3], ret[2]/ret[3] }
    end--]]

    function MM.posterior( x, eta )
	local ts = t(x)
	local gg = gradG(eta)
	return math.exp( G(eta) + ( gg[1] * (ts[1] - eta[1]) + gg[2] * (ts[2] - eta[2]) ) )
    end

    function MM.draw( lambda )
	return boxMuller(lambda[1], math.sqrt(lambda[2]))
    end

    return MM
end

-- The parameters are: Source (lambda) = p [0, 1]; Natural = theta [R]; Expectation = eta [0, 1]
function M.binomial( N )

    local MM, n  = {}, N

    local function G( eta ) return eta*math.log( eta / (n - eta) ) - n*math.log( n / (n - eta) ) end

    local function gradG( eta ) return math.log( eta / (n - eta) ) end

    function MM.eta2lambda( eta ) return eta/n end

    function MM.asstr( eta ) return eta end

    function MM.density( x, lambda)
	return choose(n, x) * math.pow( lambda, x ) * math.pow( 1-lambda, n-x)
    end

--[[    function MM.moments( lambda )
	local ret = {0, 0}
	tr.reduce( ps, function(a, p) a[1] = a[1] + p; a[2] = a[2] + 1 end, ret )
	return ret[1]/ret[2]
    end--]]

    function MM.params()
	local ret = {0}
	function ret.t( x ) return { x } end
	function ret:div( p ) return self[1]/p end
	return oo.new( params, ret )
    end

    function MM.posterior( x, eta ) return math.exp( G(eta) + (x - eta)*gradG(eta) ) end

    function MM.draw( lambda )  local cnt = 0; for i=1,n do if math.random() < lambda then cnt=cnt+1 end end; return cnt end

    return MM
end

-- The parameters are: Source (lambda) = p [0, 1]; Natural = theta [R]; Expectation = eta [0, 1]
function M.poisson( )

    local MM = {}

    local function G( eta ) return eta * math.log( eta ) - eta end

    local function gradG( eta ) return math.log( eta ) end

    function MM.eta2lambda( eta ) return eta end

    function MM.asstr( eta ) return eta end

    function MM.density( x, lambda)
	return math.pow( lambda, x ) * math.exp( -lambda ) / fact( x )
    end

    function MM.params()
	local ret = {0}
	function ret.t( x ) return { x } end
	function ret:div( p ) return self[1]/p end
	return oo.new( params, ret )
    end

    function MM.posterior( x, eta ) return math.exp( G(eta) + (x - eta)*gradG(eta) ) end

    function MM.draw( lambda )
	local l = math.exp( -lambda )
	local p = 1.0
	local k = 0
	repeat
	    k = k + 1
	    p = p *  math.random()
	until p <= l
	return k - 1
    end

    return MM
end

-- Mixture of exponential families
-- w_i | Sum[ w_i ] = 1 , w_i >= 0
-- given n observations x_i clustered into k clusters
function M.mixtureModel( EF, etas, weights )
    local N = #etas	
    local ws = weights
    if not ws then for i=1,N do ws[i] = 1/N end else assert( N == #ws, 'Number of weights should equal the number of components.') end
    local MM = {k=N, ws=ws, etas=etas, EF=EF}

    local function lambdas() return tr.map( etas, function(eta) return EF.eta2lambda( eta ) end ) end
    MM.lambdas = lambdas

    local function density( x ) return tr.reduce( lambdas(), function(a, lambda, i) a.sum = a.sum + ws[i]*EF.density(x, lambda) end, {sum=0} ).sum end
    MM.density = density

    function MM.draw( m )
        local ls = lambdas()
	local cum = tr.reduce( ws, function(a, w) a.sum = a.sum + w; a[#a+1] = a.sum end, {sum=0} )
        local ret = {}
        for i=1,m do
	    local r = math.random()
            local _,idx = tr.first( cum, function(x, j) return x>=r or j==N end ) -- replace by bisect / bsearch
	    ret[i] = EF.draw( ls[idx] )
	end
	return ret
    end

    function MM.loglikelihood( points )
	return tr.reduce( points, function(a, x) a.sum = a.sum + math.log( density(x) ) end, {sum=0} ).sum
    end

--[[
    function MM.indicator( x )
	return tr.reduce( lambdas(), function(a, lambda, i) local p = ws[i]*EF.density(x, lambda); if p > a.max then a.max=p; a.idx=i end end, {max=-1, idx=-1} ).idx
    end
--]]
    return MM
end

function M.softClustering( points, MoE )
    local N, k, EF, etas, ws, lambdas = #points, MoE.k, MoE.EF, MoE.etas, MoE.ws, MoE.lambdas
    local MAXITER = 30

    local function loglikelihood()
	local ls = lambdas()
	local function density( x ) return tr.reduce( ls, function(a, lambda, i) a.sum = a.sum + ws[i]*EF.density(x, lambda) end, {sum=0} ).sum end
	return tr.reduce( points, function(a, x) a.sum = a.sum + math.log( density(x) ) end, {sum=0} ).sum
    end

    local function EMsteps()
	local pij = tr.map( ops.range(1,N,1), function() return {} end ) -- matrix of probabilities

	for i=1,N do -- points
	    local pi = pij[i]
	    local point = points[i]
	    tr.map( etas, function(eta, j) return ws[j] * EF.posterior( point, eta) end, pi )
	    local sum = tr.reduce( pi, function(a, x) a.sum = a.sum + x end, {sum=0}).sum
            for j=1,k do pi[j] = pi[j] / sum end -- components || etas
        end
	
	for j=1,k do -- components || etas
	    local ret = tr.reduce( points, function(a, x, i) a.p = a.p + pij[i][j]; a.mu:step(x, pij[i][j]) end, {p=0, mu=EF.params()} )
    	    ws[j] = ret.p / N
    	    etas[j] = ret.mu:div( ret.p )
	end
    end

    local iter = 0
    local llh = loglikelihood()
    local thresh = math.abs(llh)*0.005

    print('\n\nInitial likelihood:', llh,'\n')

    repeat
	EMsteps();
	local llh2 = llh; llh = loglikelihood(); iter = iter + 1
	print('Iteration:', iter, '\tLikelihood:', llh)
    until iter == MAXITER or math.abs(llh2-llh) < thresh
end

--- separate bins & counts as already done by lib-histogram
function M.histoSoft( keys, counts, MoE )
    assert( #keys == #counts, 'Histogram must have same number of keys and counts' )
    local M, k, EF, etas, ws, lambdas =  #keys, MoE.k, MoE.EF, MoE.etas, MoE.ws, MoE.lambdas
    local MAXITER = 30

    local function loglikelihood()
	local ls = lambdas()
	local function density( x ) return tr.reduce( ls, function(a, lambda, i) a.sum = a.sum + ws[i]*EF.density(x, lambda) end, {sum=0} ).sum end
	return tr.reduce( keys , function(a, x, i) if counts[i] > 0 then a.sum = a.sum + counts[i]*math.log( density(x) ) end end, {sum=0} ).sum
    end

    local function EMsteps()
	local pij = tr.map( ops.range(1,M,1), function() return {} end ) -- matrix of probabilities

	-- Expectation Step
	for i=1,M do -- bins
	    local pi = pij[i]
	    local bin = keys[i]
	    if counts[i] > 0 then
	        tr.map( etas, function(eta, j) return ws[j] * EF.posterior( bin, eta) end, pi ) -- etas
	        local sum = tr.reduce( pi, function(a, x) a.sum = a.sum + x end, {sum=0}).sum -- etas
                for j=1,k do pi[j] = pi[j] / sum end -- components || etas
	    else
		for j=1,k do pi[j] = 0 end -- etas
	    end
        end
	
	local N = tr.reduce( counts, function(a,x) a.sum = a.sum + x end, {sum=0} ).sum

	for j=1,k do -- components || etas
	    local ret = tr.reduce( keys, function(a, x, i) local q = counts[i]; a.p = a.p + q*pij[i][j]; a.mu:step(x, q*pij[i][j]) end, {p=0, mu=EF.params()} ) -- CHECK THAT!!! no need for weight on counts
    	    ws[j] = ret.p / N
    	    etas[j] = ret.mu:div( ret.p )
	end
    end

    local iter = 0
    local llh = loglikelihood()
    local thresh = math.abs(llh)*0.005

--    print('\n\nInitial likelihood:', llh,'\n')

    repeat
	EMsteps();
	local llh2 = llh; llh = loglikelihood(); iter = iter + 1
--	print('Iteration:', iter, '\tLikelihood:', llh)
    until iter == MAXITER or math.abs(llh2-llh) < thresh
end

return M

