local fd = require'carlos.fold'

local ASEG = 'stats/aseg.stats'
local APARC = 'stats/lh.aparc.stats'

local asegs = {'Measure Cortex', 'Measure CerebralWhiteMatter','Left%-Cerebellum%-Cortex', 'Left%-Hippocampus', 'Right%-Cerebellum%-Cortex', 'Right%-Hippocampus'}

local aparcs = {'entorhinal', 'inferiorparietal', 'inferiortemporal', 'lateralorbitofrontal', 'medialorbitofrontal', 'middletemporal', 'parahippocampal', 'parsopercularis', 'parsorbitalis', 'parstriangularis', 'rostralmiddlefrontal', 'superiorfrontal', 'superiorparietal', 'superiortemporal', 'frontalpole', 'temporalpole', 'transversetemporal', 'insula'}

local function cuarto(line)
    if line:match'Measure' then return line else
	return line:match'%d+%.%d%s+[%w%-]+'
    end
end

local function readVars(path, vars, g)
    local f = io.open(path)
    for _,a in ipairs(vars) do
	local line = f:read'l'
	while line do
	    if line:match(a) then
		if g then print(g(line)) else print(line) end
		break
	    end
	    line = f:read'l'
	end
    end
    f:close()
end


print('ASEG\n')
readVars(ASEG, asegs, cuarto)

---[[
print('\n\nAPARC lh\n')
readVars(APARC, aparcs)

print('\n\nAPARC rh\n')
readVars(APARC:gsub('lh', 'rh'), aparcs)
--]]

