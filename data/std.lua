function CopyTable(Old)
	local New = {}
	for k,v in pairs(Old) do
		New[k] = v
	end
	return New
end