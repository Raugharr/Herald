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
	Screen:CreateTextBox("Back"):OnKey("Enter", "Released", 
		function() 
			GUI.SetMenu("MainMenu")
		end)
	DTable:CreateTextBox("First Name"):SetFocus(false)
	DTable:CreateTextBox("Last Name"):SetFocus(false)
	DTable:CreateTextBox("Age"):SetFocus(false)
	DTable:CreateTextBox("Nutrition"):SetFocus(false)
	i = 0
	for PersonData in Persons:Next() do
		if i > Columns then break end
		i = i + 1
		PersonInfo = Person(PersonData)
		DTable:CreateTextBox(PersonInfo.Name):OnKey("Enter", "Released",
		function()
			GUI.SetMenu("ViewPersonMenu", PersonInfo)
		end)
		DTable:CreateTextBox(PersonInfo.Family:GetName())
		DTable:CreateTextBox(PrintYears(PersonInfo.Age))
		DTable:CreateTextBox(PersonInfo.Nutrition)
	end
	return true
end