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

local function drawEllipse( econtour, frame, thick ) econtour:draw(frame, thick):show(200) end

local function workflow( x ) return fd.apply(x, preprocess, darkest, iseye, ellipse) end

--[[
M.inrange = inrange
M.getROI = getROI
M.preprocess = preprocess
M.darkest = darkest

return M
--]]


----- Vars -----

local movit = function() return ocv.openVideo(PATH),nil,nil end

----------------

-- Iterate until first eye-frame is found!
local frameOne, initTime = fd.first(movit, function(x) return fd.apply(x, getROI, preprocess, darkest, iseye) end)

frameOne = RECT:apply(frameOne)

print(frameOne, initTime)

----------------

local contours = fd.apply(frameOne, preprocess, darkest, function(fr) return fr:contours'CC' end)

local ellipses = fd.reduce(contours, fd.filter(inrange), fd.map(ellipse), fd.into, {})

local frameTwo = frameOne

fd.reduce(ellipses, function(e) frameTwo = e:draw(frameTwo, 3) end)

frameTwo:save'ellipses.png'

-------------------



