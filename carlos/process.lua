local ocv = require'locv'
local fd = require'carlos.fold'

local PATH='dif01.mp4'

-- FNs --

local function inrange(c)
    local a = c:property'Area'
    if a > 1000 and a < 1e5 and c:property'S' > 0.9 then
	local d = c:property('E'):dims()
	return ((1.0 * d.width) / d.height) > 0.9
    end
end

local function preprocess( frame ) return frame:togray(1):tobyte():median(11) end

local function darkest( frame ) return frame:copy( frame:range(0,80) ) end

local function iseye( frame ) return fd.first( frame:contours'CC', inrange ) end

local function ellipse( contour ) return contour:property'E' end

local function drawEllipse( econtour, frame, thick ) econtour:draw(frame, thick):show(200) end

local function workflow( x ) return fd.apply(x, preprocess, darkest, iseye, ellipse) end

----- Vars -----

local movit = function() return ocv.openVideo(PATH),nil,nil end

---------------

-- Iterate until first eye-frame is found!
local frameOne, initTime = fd.first(movit, function(x) return fd.apply(x, preprocess, darkest, iseye) end)

print(frameOne, initTime)

local ellipseOne = workflow(frameOne)

print(ellipseOne)

drawEllipse(ellipseOne, frameOne, 3)

local it = movit()

for k=1,10 do
    local tm, fr = it()
    drawEllipse(ellipseOne, fr, 2)
end
