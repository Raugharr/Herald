DebugMenu = { }

function DebugMenu.Init(Width, Height)
	local Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Persons = World.GetPersons()
	local DTable = Screen:CreateTable(4, 9, 0, {0, 0, 0, 0})
	local PersonData = nil
	local PersonInfo = ""
	
	DebugMenu.Screen = Screen
	DTable:SetCellWidth(DTable:GetFont():FontWidth() * 8)
	DTable:SetCellHeight(DTable:GetFont():FontHeight())
	Screen:CreateTextBox("Back"):OnKey("Enter", "Released", 
		function() 
			GUI.SetMenu("MainMenu")
		end)
	--print(Persons["Next"])
	DTable:CreateTextBox("First Name")
	DTable:CreateTextBox("Last Name")
	DTable:CreateTextBox("Age")
	DTable:CreateTextBox("Nutrition")
	i = 0
	for PersonData in Persons:Next() do
		if i > 8 then break end
		i = i + 1
		PersonInfo = Person(PersonData)
		DTable:CreateTextBox(PersonInfo.Name)
		DTable:CreateTextBox(PersonInfo.Family)
		DTable:CreateTextBox(IntToMonth(PersonInfo.Age))
		DTable:CreateTextBox(PersonInfo.Nutrition)
	end
	return true
end