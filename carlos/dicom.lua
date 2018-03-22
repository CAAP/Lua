--module setup
local M = {}

-- Import section
local dcm = require'lgdcm'
local ipairs = ipairs
local table = table
local print = print
local pairs = pairs
local abs = math.abs
local pi = math.pi
local acos = math.acos

local fd = require'carlos.fold'
local st = require'carlos.string'

-- No more external access after this point
_ENV = nil

-- Local variables for module-only access (private)
local tags = {}

tags.all = {
	      	{0x0008, 0x0008, 'image_type', 'TEXT'}, -- image type
	      	{0x0008, 0x0020, 'study_date', 'DATE'}, -- study date
	      	{0x0008, 0x0060, 'modality', 'TEXT'}, -- modality
	      	{0x0008, 0x1030, 'study_desc', 'TEXT'}, -- study description
	      	{0x0008, 0x103E, 'series_desc', 'TEXT'}, -- series description
		{0x0010, 0x0020, 'PID', 'TEXT'}, -- patient ID
	      	{0x0018, 0x0050, 'slice_thick', 'NUMBER'}, -- slice thickness
	      	{0x0018, 0x0080, 'TR', 'INTEGER'}, -- MRI / repetition time
	      	{0x0018, 0x0081, 'TE', 'INTEGER'}, -- MRI / echo time
	      	{0x0018, 0x0087, 'BO', 'NUMBER'}, -- MRI / magnetic field strenght
	      	{0x0018, 0x0088, 'slice_spacing', 'NUMBER'}, -- spacing between slices
	      	{0x0018, 0x1030, 'protocol', 'TEXT'}, -- protocol name
	      	{0x0018, 0x1310, 'acq_mat', 'TEXT'}, -- MRI / acquisition matrix
	      	{0x0018, 0x5100, 'patient_pos', 'TEXT'}, -- patient position
		{0x0020, 0x000D, 'OID', 'TEXT'}, -- study instance UID
		{0x0020, 0x000E, 'UID', 'TEXT'}, -- series instance UID
	      	{0x0020, 0x0011, 'series_number', 'INTEGER'}, -- series #
	      	{0x0020, 0x0013, 'image_number', 'INTEGER'}, -- image #
	      	{0x0020, 0x0032, 'IPP', 'TEXT'}, -- image position patient
	      	{0x0020, 0x0037, 'IOP', 'TEXT'}, -- image orientation patient
	      	{0x0028, 0x0010, 'rows', 'INTEGER'}, -- rows
	      	{0x0028, 0x0011, 'cols', 'INTEGER'}, -- cols
	      	{0x0028, 0x0030, 'resolution', 'TEXT'}, -- resolution: dx, dy
}

tags.keys = {'itype', 'date', 'mdty', 'stdy', 'desc', 'pid', 'thick', 'tr', 'te', 'bo', 'space', 'prot', 'mat', 'ppos', 'oid', 'uid', 'srs', 'num', 'ipp', 'iop', 'rows', 'cols', 'resol'}

fd.reduce( tags.keys, function(k, i) tags[k] = tags.all[i] end )

tags.confidential = {
		{0x0008, 0x0014, 'iUID', 'TEXT'}, -- Instance Creator UID
		{0x0008, 0x0018, 'sUID', 'TEXT'}, -- SOP Instance UID
		{0x0008, 0x0050, 'accession', 'NUMBER'}, -- Accession Number
		{0x0008, 0x0080, 'inst_name', 'TEXT'}, -- Institution Name
		{0x0008, 0x0081, 'inst_address', 'TEXT'}, -- Institution Address
		{0x0008, 0x0090, 'physician_name', 'TEXT'}, -- Referring Physician's Name
		{0x0008, 0x0092, 'physician_address', 'TEXT'}, -- Referring Physician's Address
		{0x0008, 0x0094, 'physician_phone', 'TEXT'}, -- Referring Physician's Telephone Numbers
		{0x0008, 0x1010, 'station', 'TEXT'}, -- Station Name
		{0x0008, 0x1030, 'study_desc', 'TEXT'}, -- Study Description
		{0x0008, 0x103E, 'series_desc', 'TEXT'}, -- Series Description
		{0x0008, 0x1040, 'department', 'TEXT'}, -- Institutional Department Name
		{0x0008, 0x1048, 'phys_record', 'TEXT'}, -- Physician(s) of Record
		{0x0008, 0x1050, 'perf_phys', 'TEXT'}, -- Performing Physicians' Name
		{0x0008, 0x1060, 'phys_stdy', 'TEXT'}, -- Name of Physician(s) Reading Study
		{0x0008, 0x1070, 'operator', 'TEXT'}, -- Operators' Name
		{0x0008, 0x1080, 'diagnosis', 'TEXT'}, -- Admitting Diagnoses Description
		{0x0008, 0x1155, 'rsiUID', 'TEXT'}, -- Referenced SOP Instance UID
		{0x0008, 0x2111, 'derivation', 'TEXT'}, -- Derivation Description
		{0x0010, 0x0010, 'patient_name', 'TEXT'}, -- Patient's Name
		{0x0010, 0x0020, 'patient_id', 'TEXT'}, -- Patient ID
		{0x0010, 0x0030, 'patient_bd', 'NUMBER'}, -- Patient's Birth Date
		{0x0010, 0x0032, 'patient_bt', 'NUMBER'}, -- Patient's Birth Time
		{0x0010, 0x0040, 'patient_sex', 'TEXT'}, -- Patient's Sex
		{0x0010, 0x1000, 'patient_ids', 'TEXT'}, -- Other Patient Ids
		{0x0010, 0x1001, 'patient_other', 'TEXT'}, -- Other Patient Names
		{0x0010, 0x1010, 'patient_age', 'NUMBER'}, -- Patient's Age
		{0x0010, 0x1020, 'patient_size', 'NUMBER'}, -- Patient's Size
		{0x0010, 0x1030, 'patient_weight', 'NUMBER'}, -- Patient's Weight
		{0x0010, 0x1090, 'med_record', 'TEXT'}, -- Medical Record Locator
		{0x0010, 0x2160, 'ethnic', 'TEXT'}, -- Ethnic Group
		{0x0010, 0x2180, 'occupation', 'TEXT'}, -- Occupation
		{0x0010, 0x21B0, 'patient_history', 'TEXT'}, -- Additional Patient's History
		{0x0010, 0x4000, 'patient_comments', 'TEXT'}, -- Patient Comments
		{0x0018, 0x1000, 'device_sn', 'NUMBER'}, -- Device Serial Number
		{0x0018, 0x1030, 'protocol', 'TEXT'}, -- Protocol Name
		{0x0020, 0x000D, 'study_UID', 'TEXT'}, -- Study Instance UID
		{0x0020, 0x000E, 'series_UID', 'TEXT'}, -- Series Instance UID
		{0x0020, 0x0010, 'study_ID', 'TEXT'}, -- Study ID
		{0x0020, 0x0011, 'series_nr', 'NUMBER'}, -- Series Number
		{0x0020, 0x0052, 'FoR_UID', 'TEXT'}, -- Frame of Reference UID
		{0x0020, 0x0200, 'sFoR_UID', 'TEXT'}, -- Synchronization Frame of Reference UID
		{0x0020, 0x4000, 'comments', 'TEXT'}, -- Image Comments
		{0x0040, 0x0275, 'attributes_seq', 'TEXT'}, -- Request Attributes Sequence
		{0x0040, 0xA124, 'alt_UID', 'TEXT'}, -- UID
		{0x0040, 0xA730, 'content_seq', 'TEXT'}, -- Content Sequence
		{0x0088, 0x0140, 'SMF_UID', 'TEXT'}, -- Storage Media File-set UID
		{0x3006, 0x0024, 'rFoR_UID', 'TEXT'}, -- Referenced Frame of Reference UID
		{0x3006, 0x00C2, 'qFoR_UID', 'TEXT'} -- Related Frame of Reference UID
}

--tr.reduce( tags.keys, function(a,k,i) a[k] = tags.all[i] end, tags )
fd.reduce( tags.keys, function(k, i) tags[k] = tags.all[i] end )

tags.some = {tags.pid, tags.oid, tags.uid}

-- Local function for module-only access

--http://www.itk.org/pipermail/insight-users/2003-September/004762.html
--https://groups.google.com/forum/#!topic/comp.protocols.dicom/GW04ODKR7Tc
--http://gdcm.sourceforge.net/wiki/index.php/Imager_Pixel_Spacing

-- a1 x a2
local function cross(v) return { v[2]*v[6]-v[3]*v[5] , v[3]*v[4]-v[1]*v[6] , v[1]*v[5]-v[2]*v[4] } end

-- < x | y >
local function dot( x,y ) return x[1]*y[1] + x[2]*y[2] + x[3]*y[3] end

-- cosine( 0 ) = 1.0 | cosine( 30 ) = 0.8660 | cosine( 45 ) = 0.7071 | cosine( 60 ) = 0.5
-- cosine(90) - cosine(45) ~ 0.3 | cosine(90) - cosine(30) ~ 0.13
-- < k | a1 x a2 >
local function axial(v) return abs(v[1]*v[5] - v[2]*v[4]) > 0.8 end -- increases foot to head; z
local function coronal(v) return abs(v[3]*v[4] - v[1]*v[6]) > 0.8 end -- increases posterior to anterior; y
local function sagittal(v) return abs(v[2]*v[6] - v[3]*v[5]) > 0.8 end -- increases left to right; x

-- cosine( 5 ) = 0.996194698
local function offaxis(v) if v[5] < 0.996195 then return acos(v[5]) * 180 * (v[2] > 0 and -1 or 1) / pi end end

local function cosines( img ) img.cosines = st.split( img.IOP, '\\' ); return img.cosines end

---------------------------------
-- Public function definitions --
---------------------------------

M.tags = tags

M.cosines = cosines

function M.schema( what )
    local ret = {'CREATE TABLE IF NOT EXISTS %q (path TEXT'}
    fd.reduce( what, function(x) if x[4] ~= 'SEQUENCE' then ret[#ret+1] = x[3]..' '..x[4] end end )
    ret[#ret] = ret[#ret] .. ')'
    return table.concat( ret, ', ' )
end

function M.scan( path, what, recurse )
    return dcm.scanner(path, what or tags.some, recurse )
end

function M.anonymize( path, dest )
    return dcm.anonymize( path, dest or path..'_dup' )
end

function M.confidential( )
    return dcm.profile()
end

function M.reader( what )
    return dcm.reader( what or tags.all )
end

function M.axial( img )
    return axial( img.cosines or cosines( img ) )
end

function M.coronal( img )
    return coronal( img.cosines or cosines( img ) )
end

function M.orientation( img )
    local cosines = img.cosines or cosines( img )
    if axial( cosines ) then img.orientation='axial'; return 'axial'
    elseif coronal( cosines ) then img.orientation='coronal'; return 'coronal'
    elseif sagittal( cosines ) then img.orientation='sagittal'; return 'sagittal'
    else  img.orientation='oblique'; return 'oblique' end
end

function M.normal( img ) img.normal = cross( img.cosines or cosines( img ) ); return img.normal end

function M.z( img )
    local x = st.split( img.IPP, '\\' )
    img.z = dot( cross( img.cosines or cosines( img ) ), x )
    return img.z
end

function M.resolution( img )
    local delta = st.split( img.resolution, '\\' )
    img.dx = delta[1]; img.dy = delta[2]
    return delta
end

function M.alpha( img )
    if img.orientation == 'axial' then
	local angle = offaxis( img.cosines or cosines(img) )
	img.alpha = angle or 0 -- negligable if less than 5 degrees
    else img.alpha = -1 end -- only valid for axially oriented images
    return img.alpha
end

return M
