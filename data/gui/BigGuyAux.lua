function BGStatsContainer(Guy)
	local Container = GUI.VerticalContainer(0, 0, 1000, 1000) --FIXME: shouldnt need to use a fixed max width and height.

	Container:CreateLabel("Combat: " .. Guy:GetCombat());
	Container:CreateLabel("Strength: " .. Guy:GetStrength());
	Container:CreateLabel("Toughness: " .. Guy:GetToughness());
	Container:CreateLabel("Agility: " .. Guy:GetAgility());
	Container:CreateLabel("Wit: " .. Guy:GetWit());
	Container:CreateLabel("Charisma: " .. Guy:GetCharisma());
	Container:CreateLabel("Intelligence: " .. Guy:GetIntelligence());

	Container:Shrink()
	return Container
end

function FillPersonTable(Tbl, PersonList)
	for k, v in ipairs(PersonList) do 
		Tbl:CreateLabel(v:GetName())	
		Tbl:CreateLabel(PrintYears(v:GetAge()))
		if(v:GetGender() == 1) then
			Tbl:CreateLabel("Male")
		else
			Tbl:CreateLabel("Female")
		end
	end
end

function CreatePersonTable(Parent, Cols)
	local Table = Parent:CreateTable(3, Cols + 1)

	Table:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(GUI.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name")
	Table:CreateLabel("Age")
	Table:CreateLabel("Gender")
	return Table
end

function FillRelationList(Tbl, OpinionList)
	for k, v in ipairs(OpinionList) do
		Tbl:CreateLabel(v:Action())
		Tbl:CreateLabel(v:Relation())
	end
end

function CreateAnimalTable(Parent, Cols)
	local Table = Parent:CreateTable(4, Cols + 1)


	Table:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(GUI.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name")
	Table:CreateLabel("Nutrition")
	Table:CreateLabel("Age")
	Table:CreateLabel("Gender")
	return Table
end

function CreateRelationTable(Parent, Cols)
	local Table = Parent:CreateTable(2, Cols + 1)

	Table:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(GUI.GetDefaultFont():FontHeight())
	Table:CreateLabel("Action");
	Table:CreateLabel("Modifier");
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

function GenderName(Animal)
	if Person.Male == Animal:GetGender() then
		return "Male"
	end
	return "Female"
end
