local ocv = require'locv'
local fd = require'carlos.fold'

local PATH='dif01.mp4'

-- FNs --

local function round(x) return 1000*(x/1000.0) end

local function inrange(c)
    local a = c:property'Area'
--    return a > 1000 and a < 1e5 and c:property'S' > 0.9
    if a > 1000 and a < 1e5 and c:property'S' > 0.9 then
	local d = c:property('E'):dims()
	return ((1.0 * d.width) / d.height) > 0.9
    end
end

local function oneFrame(frame, k)
    local img = frame:togray(1):tobyte():median(11)
    local eye = img:copy( img:range(0,80) )
    local c = fd.first( eye:contours'CC', inrange )

    if c then
	local e = c:property'E'
	e:draw(frame, 3):show()
	return true
    else return nil
    end
end

--------------------

local movit = ocv.openVideo(PATH)

local fr = fd.first(function() return movit, nil, nil end, oneFrame)




