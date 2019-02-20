local fd = require'carlos.fold'

local ASEG = 'stats/aseg.stats'
local APARC = 'stats/lh.aparc.stats'

local asegs = {'Measure Cortex', 'Measure CerebralWhiteMatter','Measure TotalGray', 'Left%-Cerebellum%-Cortex', 'Left%-Caudate', 'Left%-Putamen', 'Left%-Pallidum', 'Left%-Hippocampus', 'Left%-Amygdala', 'Right%-Cerebellum%-Cortex', 'Right%-Caudate', 'Right%-Putamen', 'Right%-Pallidum', 'Right%-Hippocampus', 'Right%-Amygdala'}

local aparcs = {'caudalanteriorcingulate', 'entorhinal', 'inferiorparietal', 'inferiortemporal', 'lateralorbitofrontal', 'medialorbitofrontal', 'middletemporal', 'parahippocampal', 'parsopercularis', 'parsorbitalis', 'parstriangularis', 'posteriorcingulate', 'rostralanteriorcingulate', 'rostralmiddlefrontal', 'superiorfrontal', 'superiorparietal', 'superiortemporal', 'frontalpole', 'temporalpole', 'transversetemporal', 'insula'}

local function cuarto(line)
    if line:match'Measure' then return line else
	return line:match'%d+%s+%d+%.%d%s+[%w%-]+'
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

