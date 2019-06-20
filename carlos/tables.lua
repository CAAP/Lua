-- module setup
local M = {}

-- Import Section
local ipairs=ipairs
local pairs=pairs
local concat=table.concat
local sort=table.sort
local assert=assert
local type=type
local sqrt=math.sqrt
local print=print
local huge=math.huge 
local rand=math.random

local tr = require'carlos.transducers'

-- Local Variables for module-only access

-- No more external access after this point
_ENV = nil -- or M

-- Function definitions

local function drop(atable, n, m)
  assert(n<#atable, 'Number of elements to drop exceeds table dimension.')

  local ret = {}
  local M = m or #atable

  assert(n<M, "Last element index, less than first element index.")

  for i=n,M do
    ret[#ret+1] = atable[i]
  end

  return ret
end

M.drop = drop

function M.take(atable, n)
    return tr.filter( atable, function(_,k) if k < n+1 then return true end end )
end

--[[
  local M = n>#atable and #atable or n
  local ret = {}

  for i=1,M do
    ret[i] = atable[i]
  end

  return ret
end
--]]

-- takes a table and returns a map
-- key is the first element in each row
-- value is the rest
function M.asmap(atable)
  local ret={}

  for _,row in ipairs(atable) do
    ret[row[1]] = (#row>2) and drop(row,2) or row[2]
  end

  return ret
end

function M.append(atable, another, offset, start, stop)
  local a = (start or 1) - 1
  local b = stop or #another[1]
  local N = b - a
  local c = (offset or #atable[1])

  for i,v in ipairs(atable) do
    for j=1,N do
      v[c+j] = another[i][j+a]
    end
  end
end

function M.join(atable, another)
  local ret = {}

  for i,v in ipairs(atable) do
    ret[i] = v
  end

  local a=#ret
  for i,v in ipairs(another) do
    ret[a+i] = v
  end

  return ret
end

function M.multiply(atable, scale, start, stop)
  local ret = {}
  local a = (start or 1) - 1
  local b = stop or #atable[1]

  assert(a<b, 'Last index cannot be less than start index.')

  for i,v in ipairs(atable) do
    ret[i] = {}
    for j=1,a do
      ret[i][j] = v[j]
    end
    for j=a+1,b do
      ret[i][j] = v[j]*scale
    end
    for j=b+1,#v do
      ret[i][j] = v[j]
    end
  end

  return ret
end

local function selection(atable, closure, indices, offset)
  local a = offset or 0

  for i,v in ipairs(atable) do
    for _,j in ipairs(indices) do
      v[j+a] = closure(v[j+a])
    end
  end
end

M.selection = selection

function M.asstr(atable, sep)
  local dlm=sep or ','

  assert(type(atable[1]) == 'table', 'A normal table given, not a hypertable.')

  local ret = tr.map( atable, function(row) return concat(row, dlm) end )

  return concat(ret,'\n')
end

-- Fisher-Yates shuffle algorithm (in-place)
function M.shuffle(atable)
  local N = #atable
  for i=1,N-1 do
    local j = rand(i,N)
    atable[j], atable[i] = atable[i], atable[j]
  end
  return atable
end

-- takes a table and returns a binned version of the data
function M.bin(atable, width)
    local ret = {}

    for i,v in ipairs(atable) do
	if type(v) == 'table' then
	    ret[i] = {}
	    for j,w in ipairs(v) do
	        ret[i][j] = width[j]*floor(w/width[j])
            end
	else
	    ret[i] = width*floor(v/width)
	end
    end

    return ret
end

-- takes a table and return the rows in indices
function M.collect(atable, indices)
    local ret = {}

    for i,v in ipairs(indices) do
	ret[i] = atable[v]
    end

    return ret
end

-- takes two tables; mapping creates a HashMap from tableKeys; byUsing do the merging
function M.merge(tableDest, tableKeys, mapping, byUsing)
    local amap = mapping(tableKeys)

    for _,x in ipairs(tableDest) do
        byUsing(x, amap)
    end
end

function M.transpose(atable)
  local ret = {}
  local M = #atable
  local N = #atable[1]

  for j=1,N do
    ret[j] = {}
    for i=1,M do
      ret[j][i] = atable[i][j]
    end
  end

  return ret
end

local function variance(atable, start, stop)
    -- wikipedia --
    local mean = {}
    local m2 = {}
    local sigma = {}
    local R = #atable -- number of points; rows

    local a = (start or 1) - 1
    local b = stop or #atable[1]
    local N = b-a -- number of features

    for j=1,N do
	mean[j] = 0
	m2[j] = 0
    end

    for i=1,R do
	for j=1,N do
	    local delta = atable[i][j+a] - mean[j]
	    mean[j] = mean[j] + delta/i
	    m2[j] = m2[j] + delta*( atable[i][j+a] - mean[j])
	end
    end

    for j,v in ipairs(m2) do
	sigma[j] = v/(R-1)
    end

    return sigma, mean, R 
end

M.variance = variance

function M.zscore(atable)
    local ret = {}
    local R = #atable -- number of points; rows
    local N = #atable[1] -- number of variables; columns
    local sigma,mean = variance(atable)

    for i=1,R do
	ret[i] = {}
    end

    for j=1,N do
	local m = mean[j]
	local s = sqrt(sigma[j])
	for i=1,R do
	    ret[i][j] = (atable[i][j]-m)/s
	end
    end

    return ret
end

local function max(atable, start, stop)
  local ret = {}
  local a = (start or 1) - 1
  local b = stop or #atable[1]
  local N = b - a

  for j=1,N do
    ret[j] = -huge
  end

  for _,v in ipairs(atable) do
    for j=1,N do
      if(v[j+a]>ret[j]) then ret[j] = v[j+a]  end
    end
  end

  return ret
end

M.max = max

function M.normalize(atable, start, stop)
  local mx = max(atable, start, stop)
  local a = (start or 1) - 1

  for _,v in ipairs(atable) do
    for j,z in ipairs(mx) do
      v[j+a] = v[j+a] / z
    end
  end
end

function M.scale(atable, scale)
    local a = scale or 1
    local ret = {}

    if type(a) == 'table' then
	assert(#atable[1] == #a,'Number of scale factors must match number of variables.')
	for i, v in ipairs(atable) do
	    ret[i] = {}
	    for j, x in ipairs(v) do
	        ret[i][j] = a[j]*x
	    end
        end
    else
	for i, v in ipairs(atable) do
	    ret[i] = {}
	    for j, x in ipairs(v) do
	        ret[i][j] = a*x
	    end
        end
    end

    return ret
end

-- normalized; one-dimensional histograms only --
local function cumulative(atable)
    for _, hist in ipairs(atable) do
	local acc = 0
	for i, ent in ipairs(hist) do
	    acc = acc + ent[2]
	    ent[2] = acc
	end
	for _, ent in ipairs(hist) do
	    ent[2] = ent[2] / acc -- normalization [0,1]
	end
    end
end

local function map2table(amap)
    local ret = {}
    for x,_ in pairs(amap) do
	ret[#ret+1] = x
    end
    return ret
end

M.map2table = map2table

function M.extrema(matrix, idx)
    local ret = {}
    local i = idx or 1
    
    for _,v in ipairs(matrix) do
	ret[#ret+1] = v[i]
    end

    local N = #ret

    assert(N>0, 'The matrix is empty.')
    
    sort(ret)
    
    local mdn = N%2==1 and ret[(N+1)/2] or (ret[N/2]+ret[N/2+1])/2

    print('Index = ' .. i)
    print('N = ' .. N)
    print('Min = ' .. ret[1])
    print('Max = ' .. ret[N])
    print('Median = ' .. mdn)

    return {size=N, min=ret[1], max=ret[N], median=mdn}
end

return M
