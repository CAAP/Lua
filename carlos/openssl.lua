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

function M.md2( msg ) return dgs.digest('md2', msg) end

function M.md5( msg ) return dgs.digest('md5', msg) end

function M.md5sha1( msg) return dgs.digest('md5_sha1', msg) end

function M.sha1( msg ) return dgs.digest('sha1', msg) end

function M.sha224( msg ) return dgs.digest('sha224', msg) end

function M.sha384( msg ) return dgs.digest('sha384', msg) end

function M.sha512( msg ) return dgs.digest('sha512', msg) end

function M.dss( msg ) return dgs.digest('dss', msg) end

function M.dss1( msg ) return dgs.digest('dss1', msg) end

function M.ripemd160( msg ) return dgs.digest('ripemd160', msg) end

return M 
