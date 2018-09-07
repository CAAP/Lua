-- module setup
local M = {}

-- Import Section

local concat = table.concat

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access

local REGEX = "(%$[%u_]+)"

--------------------------------
-- Local function definitions --
--------------------------------

local function google_fonts( family )
    local ret = [=[<link href="https://fonts.googleapis.com/css?family=%s" rel="stylesheet">]=]
    return string.format(ret, family)
end

local function extended_components( a )
-- https://cdn.ampproject.org
-- pattern of /v\d+/[a-z-]+-(latest|\d+|\d+\.\d+)\.js
    local ret = "https://cdn.ampproject.org/$RUNTIME_VERSION/$ELEMENT_NAME-$ELEMENT_VERSION.js"
    return ret:gsub(ret, a)
end

local function image( a )
    local DEFS = {['$SRC']="", ['$ALT']="", ['$HEIGHT']="", ['$WIDTH']="", ['$LAYOUT']="responsive", ['$MEDIA']="(max-width: 415px)", ['$URL']=""}
-- $SRC, $ALT, $HEIGHT, $WIDTH, $LAYOUT, $MEDIA
    local im = [=[<amp-img src="$SRC" alt="$ALT" height="$HEIGHT" width="$WIDTH" layout="$LAYOUT" media="$MEDIA"></amp-img>]=]
-- $URL, $MEDIA
    local ln = [=[<link rel="preload" as="image" href="$URL" media="$MEDIA">]=]

    im = im:gsub(REGEX, a):gsub(REGEX, DEFS)
    ln = ln:gsub(REGEX, a):gsub(REGEX, DEFS)

    return ln, im
end

---------------------------------
-- Public function definitions --
---------------------------------

function M.html()
    local MM = {}
-- $LANG, $TITLE, $HTML_URL, $SCRIPT, $STYLE, $BODY, $CUSTOM, $EXTENDED, $FONTS, $IMAGES
    local DEFS = {['$LANG']="es", ['$TITLE']="Test Page", ['$HTML_URL']=".", ['$SCRIPT']="", ['$BODY']="", ['$CUSTOM']="", ['$EXTENDED']="", ['$FONTS']="", ['$IMAGES']=""}

    local bodyb = {}
    local fontsb = {}
    local imagesb = {}

    local ret = [==[
    <!doctype html>
    <html amp lang="$LANG">
      <head>
        <meta charset="utf-8">
	<meta name="viewport" content="width=device-width,minimum-scale=1,initial-scale=1">
	<link rel="preload" as="script" href="https://cdn.ampproject.org/v0.js">
	<link rel="preconnect dns-prefetch" href="https://fonts.gstatic.com/" crossorigin>
	<title>$TITLE</title>
	<link rel="canonical" href="$HTML_URL">
	<script async src="https://cdn.ampproject.org/v0.js"></script>
	<style amp-custom>
	  $CUSTOM
	</style>
	<script type="application/ld+json">
	  $SCRIPT
	</script>
	$EXTENDED
	$FONTS
	$IMAGES
	<style amp-boilerplate>body{-webkit-animation:-amp-start 8s steps(1,end) 0s 1 normal both;-moz-animation:-amp-start 8s steps(1,end) 0s 1 normal both;-ms-animation:-amp-start 8s steps(1,end) 0s 1 normal both;animation:-amp-start 8s steps(1,end) 0s 1 normal both}@-webkit-keyframes -amp-start{from{visibility:hidden}to{visibility:visible}}@-moz-keyframes -amp-start{from{visibility:hidden}to{visibility:visible}}@-ms-keyframes -amp-start{from{visibility:hidden}to{visibility:visible}}@-o-keyframes -amp-start{from{visibility:hidden}to{visibility:visible}}@keyframes -amp-start{from{visibility:hidden}to{visibility:visible}}</style><noscript><style amp-boilerplate>body{-webkit-animation:none;-moz-animation:none;-ms-animation:none;animation:none}</style></noscript>
      </head>
      <body>
        $BODY
      </body>
    </html>
    ]==]

    function MM.lang( lng ) DEFS['$LANG'] = lng; return MM; end

    function MM.title( ttl ) DEFS['$TITLE'] = ttl; return MM; end

    function MM.html_url( url ) DEFS['$HTML_URL'] = url; return MM; end

    function MM.script( scr ) DEFS['$SCRIPT'] = scr; return MM; end

    function MM.style( stl ) DEFS['$STYLE'] = stl; return MM; end

    function MM.append_body( s ) bodyb[#bodyb+1] = s; return MM; end

    function MM.custom( stl ) DEFS['$CUSTOM'] = stl; return MM; end

    function MM.extended( scr ) DEFS['$EXTENDED'] = scr; return MM; end

    function MM.append_font( fnt ) fontsb[#fontsb+1] = fnt; return MM; end

    function MM.append_image( img )
	local imtxt, bdtxt = image( img )
	imagesb[#imagesb+1] = imtxt
	bodyb[#bodyb+1] = bdtxt
	return MM
    end

    function MM.asstr()
	DEFS['$BODY'] = concat(bodyb, '\n')
	DEFS['$FONTS'] = concat(fontsb, '\n')
	DEFS['$IMAGES'] = concat(imagesb, '\n')
	return ret:gsub(REGEX, DEFS)
    end

    return MM
end


return M

