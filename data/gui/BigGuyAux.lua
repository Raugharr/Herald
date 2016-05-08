function BGStatsContainer(Guy)
	local Container = GUI.VerticalContainer(0, 0, 1000, 1000) --FIXME: shouldnt need to use a fixed max width and height.

	Container:CreateLabel("Administration: " .. Guy:GetAdministration());
	Container:CreateLabel("Intrigue: " .. Guy:GetIntrigue());
	Container:CreateLabel("Strategy: " .. Guy:GetStrategy());
	Container:CreateLabel("Warfare: " .. Guy:GetWarfare());
	Container:CreateLabel("Tactics: " .. Guy:GetTactics());
	Container:CreateLabel("Charisma: " .. Guy:GetCharisma());
	Container:CreateLabel("Piety: " .. Guy:GetPiety());
	Container:CreateLabel("Intellegence: " .. Guy:GetIntellegence());

	Container:Shrink()
	return Container
end

function FillPersonTable(Tbl, PersonList)
	for k, v in ipairs(PersonList) do 
		Tbl:CreateLabel(v:GetName())	
		Tbl:CreateLabel(PrintYears(v:GetAge()))
	end
end

function CreatePersonTable(Parent, Cols)
	local Table = Parent:CreateTable(2, Cols)

	Table:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(GUI.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name");
	Table:CreateLabel("Age");
	return Table
end

function FillList(ItrFunc, ItrArg)
	local Table = {}
	local Idx = 1

	for v in ItrFunc(ItrArg) do
		Table[Idx] = v
		Idx = Idx + 1
	end
	return Table
end
