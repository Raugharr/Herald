ManorConstraints = CreateConstraint(50, 800, 50)
BabyAvg = {0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999}
AgeGroups = {5, 0, 2190, 4745, 5659, 21900, 36500}

function CopyTable(Old)
	local New = {}
	for k,v in pairs(Old) do
		New[k] = v
	end
	return New
end