GameMenu = { }

function GameMenu.Init(Width, Height)
	local Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Menu = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0}, Screen)
	local DateCont = GUI.VerticalContainer(0, 0, 512, 100, 0, {0, 0, 0, 0}, Screen) 
	
	Menu:CreateTextBox("View Settlement")
	Menu:CreateTextBox("Save")
	Menu:CreateTextBox("Advance Month")
	Menu:CreateTextBox("Back"):OnKey("Enter", "Released",
		function() 
			GUI.PopMenu() 
		end)
		
	DateCont:CreateTextBox(PrintDate(World.GetDate()))
	return true
end
