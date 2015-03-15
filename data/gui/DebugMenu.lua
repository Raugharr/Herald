DebugMenu = { }

function DebugMenu.Init(Width, Height, Data)
	local Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Persons = World.GetPersons()
	local Columns = 16
	local DTable = Screen:CreateTable(4, Columns, 0, {0, 0, 0, 0})
	local PersonData = nil
	local PersonInfo = ""
	
	DebugMenu.Screen = Screen
	DTable:SetCellWidth(DTable:GetFont():FontWidth() * 8)
	DTable:SetCellHeight(DTable:GetFont():FontHeight())
	Screen:CreateLabel("Back"):OnKey("Enter", "Released", 
		function() 
			GUI.PopMenu()
		end)
	DTable:CreateLabel("First Name"):SetFocus(false)
	DTable:CreateLabel("Last Name"):SetFocus(false)
	DTable:CreateLabel("Age"):SetFocus(false)
	DTable:CreateLabel("Nutrition"):SetFocus(false)
	i = 0
	for PersonData in Persons:Next() do
		if i > Columns then break end
		i = i + 1
		PersonInfo = Person(PersonData)
		DTable:CreateLabel(PersonInfo.Name):OnKey("Enter", "Released",
		function()
			GUI.SetMenu("ViewPersonMenu", PersonInfo)
		end)
		DTable:CreateLabel(PersonInfo.Family:GetName())
		DTable:CreateLabel(PrintYears(PersonInfo.Age))
		DTable:CreateLabel(PersonInfo.Nutrition)
	end
	return true
end
