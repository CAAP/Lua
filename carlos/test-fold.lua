local fd = require'carlos.fold'

-- find even numbers in range [1..10]
-- test: times, into, comp, filter

local even = fd.times( 10, fd.filter(function(x) return x % 2 == 0 end), fd.into, {} )
for i=1,5 do assert( even[i] == i*2, 'Even number mismatch: ' .. i*2 .. '\t' .. even[i] ) end

fd.times( 10, fd.comp{ fd.filter(function(x) return x%2 == 0 end), fd.into(even) } )
for i=1,5 do assert( even[i] == even[i+5], 'Even number mismatch: ' .. even[i] .. '\t' .. even[i+5] ) end

--
-- test: reduce

local odd = fd.reduce( even, function(a) return function(x, i) a[i] = x-1 end end, {} )
for i=1,5 do assert( odd[i] == i*2-1, 'Odd number mismatch: ' .. i*2 - 1 .. '\t' .. odd[i] ) end

--
-- test: merge, rejig

local evenp = fd.reduce( even, fd.rejig(function(x) return x,x end), fd.merge, {} )
for i=2,10,2 do assert( evenp[i] == i, 'Even number mismatch ' .. i .. '\t' .. evenp[i] ) end

local evenp = fd.reduce( even, fd.rejig(function(x) return true,x end), fd.merge, {} )
for i=2,10,2 do assert( evenp[i], 'Even number not found ' .. i ) end

