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

local function distill(data)
    local M = #data
    local N = fd.reduce(data, fd.map(function(x) return #x end), fd.sum, {sum=0}).sum

    local ys = fd.reduce(data, fd.map(function(x) return x[1] end), fd.into, {})
    local nodes = nodes(data, N)

    local MM = {}

    function MM.get(m)
	assert(m < N, 'Index must be less than number of nodes')
	return nodes:get(m)
    end

    function MM.__len() return M, N end



    return MM
end




local function svmfeats(atable, classIdx, negClass, start, stop)
  local ret = {}
  local ci = classIdx or 1
  local a = (start or 2) - 1
  local b = stop or #atable[1]
  local N = b - a -- number of variables; columns
  local ng = negClass or 0

  assert(ci<=a or ci>b, 'ClassIndex is included in data indices.')

  for i,v in ipairs(atable) do
    ret[i] = {v[ci] == ng and -1 or 1}
    for j=1,N do
      ret[i][2*j] = j
      ret[i][2*j+1] = v[j+a]
    end
  end

  return ret
end





M.svmfeats = svmfeats

function M.selected(atable, classIdx, start, selection)
  local ci = classIdx or 1
  local a = (start or 2) - 1

  assert(ci<a or ci>b, 'ClassIndex is included in data indices.')

  local ret = {}
  for i,v in ipairs(atable) do
    ret[i] = {v[ci]}
    for j,k in ipairs(selection) do
      ret[i][j+1] = v[k+a]
    end
  end

  return ret
end

local function preprocess(atable, classIdx, start, stop)
  local ci = classIdx or 1
  local a = (start or 2) - 1
  local b = stop or #atable[1]
  local N = b - a -- number of variables; columns

  assert(ci<a or ci>b, 'ClassIndex is included in data indices.')

  local ret = {}
  for i,v in ipairs(atable) do
    ret[i] = {v[ci]}
    for j=1,N do
      ret[i][j+1] = v[j+a]
    end
  end

  return ret
end

local function problem(atable, classIdx, negClass, start, stop)
  return svmprob(svmfeats(atable, classIdx, negClass, start, stop))
end

M.problem = problem

function M.weights(amodel)
  local ws=amodel:weights()
  local mx=-huge

  -- find maximum value
  for i=1,#ws do
    ws[i] = abs(ws[i])
    if ws[i]>mx then mx=ws[i] end
  end

  -- scale every weight : [0, 1]
  for i=1,#ws do
    ws[i] = ws[i]/mx
  end

  return ws
end

function M.rfe(atable, classIdx, negClass, start, stop)
  local ret = preprocess(atable, classIdx, start, stop)
  local N = #ret[1] - 1 -- number of variables; first column correspond to class/label
  local ng = negClass or 0

  local index = {}
  for i=1,N do
    index[i]= i
  end

  local b = params()
  b['t'] = 0 -- linear classifier

  local rank = {}
  for i=1,N-1 do
    local a = problem(ret, 1, ng)
    local c = train(a,b)
    local ws=c:weights()

    local k = reduce(ws, function(x,y) return pow(x,2)<pow(y,2) end)

    rank[remove(index, k)] = i
    for _,v in ipairs(ret) do
      remove(v, k+1)
    end
  end

  rank[index[1]] = N

  return rank
end

-- list of: target, predicted
function M.performance(atable)
  local N = #atable
  local correct = {} 
  local incorrect = {}

  -- initialize counts
  correct[1] = 0
  incorrect[1] = 0
  correct[-1] = 0
  incorrect[-1] = 0

  for _,v in ipairs(atable) do
    if v[1] == v[2] then
      correct[v[1]] = correct[v[1]] + 1
    else
      incorrect[v[2]] = incorrect[v[2]] + 1
    end
  end

  --TruePositive, TrueNegative, FalsePositive, FalseNegative
  return correct[1], correct[-1], incorrect[1], incorrect[-1]
end

function M.binary(tp, tn, fp, fn)
  return tp/(tp+fn), tn/(tn+fp), (tp+tn)/(tp+tn+fp+fn)
end

--[[
function M.ranked(rank, rankIdx, atable, classIdx, negClass, start)
  local N = #rank -- number of variables; columns
  local ng = negClass or 0

  local b = params()
  b['t'] = 0 -- linear

  local order = {}
  for i=1,N do
    order[i] = i
  end

  -- sorted from top ranked to lowest
  sort(order, function(x,y) return rank[x][rankIdx] > rank[y][rankIdx] end)

  local ret = sorted(atable, classIdx, start, order)

  local perf = {{'n(features)', 'TP', 'TN', 'FP', 'FN'}}
  for i=1,N-1 do
    local M = #ret[1] - 1 -- 1st feature is class/label
    local a = svmfeats(ret, 1, ng, 1, M)
    randomseed(1)
    local c = crossv(a, b, 10)

    perf[i+1] = {M, unpack(performance(c))}
    
    for _,v in ipairs(ret) do
      remove(v) -- remove feature with smallest rank
    end
  end

  return perf
end
--[[
function M.selected(features, atable, classIdx, negClass, start)
  local N = #features -- number of variables; columns
  local ng = negClass or 0

  local b = params()
  b['t']=0 -- linear

  local ret = sorted(atable, classIdx, start, features)
  local a = svmfeats(ret, 1, ng, 1, N)
  randomseed(1)
  local c = crossv(a, b, 10)

  return performance(c)
end
--]]
return M
