-- module setup
local M = {}

-- Import Section
local leyes = require'leyes'
local locv = require'locv'
local fd = require'carlos.fold'

local asint = math.tointeger

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

--------------------------------
-- Local function definitions --
--------------------------------

local function ocvImage(s, d, ch)
    local depth = s:match("depth = '(%w+)'")
    local channels = asint(s:match("channels = (%d)"))
    local a = true
    if d then a = depth == d end
    if a and ch then a = channels == ch end
    return a
end

local function inrange(c)
    local a = c:property'Area'
    if a > 1000 and a < 1e5 and c:property'S' > 0.9 then
	local d = c:property('E'):dims()
	return ((1.0 * d.width) / d.height) > 0.85
    end
end

local function ellipse( contour ) return contour:property'E' end

local function drawEllipse( econtour, frame, thick ) return econtour:draw(frame, thick) end

local function getCoords( econtour ) local d = econtour:dims(); return d.x, d.y end

---------------------------------
-- Public function definitions --
---------------------------------

function M.video(s)
    local iter = locv.openVideo(s)
    local MM = {}

    function MM.first()
	if MM.time then return MM end
	local fr, tm = fd.first(function() return iter,nil,nil end, function(x) return fd.apply(x, M.preprocess, M.darkest, M.iseye) end)
	MM.time = tm; MM.frame = fr
	return MM
    end

    function MM.next()
	local tm, fr = iter()
	if fr then
	    MM.time = tm; MM.frame = fr
	    return MM
	else
	    return nil
	end
    end

    return MM
end

function M.image(s) return locv.open(s) end

function M.preprocess(img)
    local ppties = img:__tostring()
    return ocvImage(ppties, '8U', 1) and img:median(11) or img:togray(1):tobyte()
end

function M.darkest(img) return img:copy( img:range(0, 80) ) end

function M.iseye(img) return fd.first( img:contours'CC', inrange ) end

M.ellipse = ellipse

M.drawEllipse = drawEllipse

function M.gradiente(img) return leyes.eyes(img) end

return M
