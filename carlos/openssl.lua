--module setup
local M = {}

-- Import section
local dgs = require'ldgst'

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)

-- Local functions for module-only access

---------------------------------
-- Public function definitions --
---------------------------------

function M.md5( msg ) return dgs.digest('md5', msg) end

function M.sha1( msg ) return dgs.digest('sha1', msg) end

function M.sha256( msg ) return dgs.digest('sha256', msg) end

return M 
