local ocv = require'locv'
local fd = require'carlos.fold'

local PATH='fesormex.mp4'

local RECT = ocv.rect{x=0, y=0, width=1920, height=500}

local M = {RECT=RECT}

-- FNs --

local function inrange(c)
    local a = c:property'Area'
    if a > 1000 and a < 1e5 and c:property'S' > 0.9 then
	local d = c:property('E'):dims()
	return ((1.0 * d.width) / d.height) > 0.85
    end
end

local function getROI( frame ) return RECT:apply(frame) end

local function preprocess( frame ) return frame:togray(1):tobyte():median(11) end

local function darkest( frame ) return frame:copy( frame:range(0,95) ) end

local function iseye( frame ) return fd.first( frame:contours'CC', inrange ) end

local function ellipse( contour ) return contour:property'E' end

local function drawEllipses( econtours, frame, thick ) fd.reduce(econtours, function(e) frame = e:draw(frame, thick or -1) end); return frame; end

local function workflow( x ) return fd.apply(x, preprocess, darkest, iseye, ellipse) end

local function lookForEllipses(fr)
    local cts = fd.apply(fr, preprocess, darkest, function(f) return f:contours'CC' end)
    return fd.reduce(cts, fd.filter(inrange), fd.map(ellipse), fd.into, {})
end

----- Vars -----

local movit = function() return ocv.openVideo(PATH),nil,nil end

-- Iterate until first eye-frame is found!
local frameOne, initTime = fd.first(movit, function(x) return fd.apply(x, getROI, preprocess, darkest, iseye) end)

frameOne = RECT:apply(frameOne)

print(frameOne, initTime)

----------------

-- Recover contour(s) from frameOne & draw ellipses
local frTwo = frameOne

local es = lookForEllipses( frTwo )

--frTwo = drawEllipses( es, frTwo, 3 )

--local frTwo = frTwo:save'ellipses.png'

print('Coords are:\n', table.concat(fd.reduce(es, fd.map(function(e) local d = e:dims(); return string.format('\t%0.2f, %0.2f', d.x, d.y) end), fd.into, {}), '\n'))

local maskedE = frameOne:zeros()

maskedE = drawEllipses( es, maskedE )

--maskedE:save'masked.png'

-------------------

---[[
local iter = movit()
maskedE = maskedE:morphology('Dilate', 'Ellipse', 20)

for k=1,10 do
    local t, fr =  iter()

    fr = fd.apply(fr, getROI, preprocess, function(f) return f:overlay(maskedE) end)
    fr:save(string.format("%03d.png", k))
    print("Time: ", t)
end
--]]


