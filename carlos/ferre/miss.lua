
-- Import Section
--
local fd	= require'carlos.fold'

local format	= require'string'.format

local print	= print

-- No more external access after this point
_ENV = nil -- or M

-- Local Variables for module-only access
--

local DATA	 = {}

--------------------------------
-- Local function definitions --
--------------------------------
--
 -- cmds: tabs, delete, msgs
local function update(pid, cmd, msg)
    if not PIDS[pid] then PIDS[pid] = {} end
    local p = PIDS[pid]
    if cmd == 'delete' and p.tabs then p.tabs = nil
    else p[cmd] = '%s ' .. msg end -- add fruit
end

local function switch( msg, msgr )

    local cmd = msg:match'%a+'
    if cmd == 'pins' then
	PINS.store(pid, msg)
	msgr:send_msg( msg )
    elseif cmd == 'login' then
	local fruit = msg:match'fruit=(%a+)'
	local ft = FRUITS[pid]
	if ft and ft ~= fruit then msgr:send_msg(format('%s logout pid=%d', ft, pid)) end
	FRUITS[pid] = fruit
	if PIDS[pid] then
	    fd.reduce(fd.keys(PIDS[pid]), function(m) msgr:send_msg(format(m, fruit)) end)
	end
    else -- tabs, delete, msgs
	update(pid, cmd, msg)
	local ft = FRUITS[pid]
	if cmd == 'msgs' and ft then msgr:send_msg(format('%s %s', ft, msg)) end
    end
    print(msg, '\n')
    ::FIN::

end

return switch
