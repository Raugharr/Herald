function BGStatsContainer(Guy)
	local Container = Gui.VerticalContainer(0, 0, 1000, 1000) --FIXME: shouldnt need to use a fixed max width and height.

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
	local Skin = Parent:GetSkin()
	local Font = Skin:Table():GetFont()
	
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	Table:CreateLabel("Name")
	Table:CreateLabel("Age")
	Table:CreateLabel("Gender")
	return Table
end

function CreateWarriorTable(Parent, Cols)
	local Table = Parent:CreateTable(2, Cols + 1)

	Table:SetCellWidth(Gui.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(Gui.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name")
	Table:CreateLabel("Age")
	return Table
end

function FillWarriorTable(Tbl, PersonList)
	for v in PersonList:Itr():Next() do 
		Tbl:CreateLabel(v:GetName())	
		Tbl:CreateLabel(PrintYears(v:GetAge()))
	end
end

function FillRelationList(Tbl, OpinionList)
	for k, v in ipairs(OpinionList) do
		Tbl:CreateLabel(v:Action())
		Tbl:CreateLabel(v:Relation())
	end
end

function CreateAnimalTable(Parent, Cols)
	local Table = Parent:CreateTable(4, Cols + 1)


	Table:SetCellWidth(Gui.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(Gui.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name")
	Table:CreateLabel("Nutrition")
	Table:CreateLabel("Age")
	Table:CreateLabel("Gender")
	return Table
end

function CreateRelationTable(Parent, Cols)
	local Table = Parent:CreateTable(2, Cols + 1)

	Table:SetCellWidth(Gui.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(Gui.GetDefaultFont():FontHeight())
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

function GeneralActions(Container, Owner, Target)
	Container:CreateButton("Raise Popularity", 
		function()
			Owner:SetAction(Action.RaisePop, nil)
		end)
	Container:CreateButton("Raise Glory",
		function()
			Owner:SetAction(Action.RaiseGlory, nil)
		end)
	if Owner == Target or Target == nil then
		return
	end
	Container:CreateButton("Steal",
		function()
			Owner:SetAction(Action.Steal, Target)
		end)
	Container:CreateButton("Befriend", 
		function()
			Owner:SetAction(Action.Befriend, Target)
		end)
	Container:CreateButton("Murder",
		function()
			Owner:SetAction(Action.Murder, Target)
		end)
	Container:CreateButton("Duel", 
		function()
			Owner:SetAction(Action.Duel, Target)
		end)

end
