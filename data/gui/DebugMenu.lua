Menu.__savestate = true;
Menu.moveable = true;

function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Persons = World.GetPersons()
	local Columns = 16
	local DTable = Screen:CreateTable(4, Columns, 0, {0, 0, 0, 0})
	local PersonData = nil
	local PersonInfo = ""
	
	Menu.DTable:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
	Menu.DTable:SetCellHeight(GUI.GetDefaultFont():FontHeight())
	Menu.Screen:CreateLabel("Back"):OnKey("Enter", "Released", 
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
	return Menu.Screen
end
