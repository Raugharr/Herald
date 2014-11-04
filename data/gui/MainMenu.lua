MainMenu = { }

function MainMenu.Init(Width, Height, Data)
	MainMenu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 2, {50, 50, 0, 0})
	
	GUI.BackgroundColor(0, 0, 0)
	Title = MainMenu.Screen:CreateTextBox("Herald"):SetFocus(false)
	MainMenu.Screen:CreateTextBox("New"):OnKey("Enter", "Released",
		function()
			GUI.SetMenu("GameMenu")
		end)
	MainMenu.Screen:CreateTextBox("Load")
	MainMenu.Screen:CreateTextBox("Debug"):OnKey("Enter", "Released", 
		function() 
			GUI.SetMenu("DebugMenu") 
		end)
	MainMenu.Screen:CreateTextBox("Exit"):OnKey("Enter", "Released", 
		function() 
			GUI.CloseMenu()
		end)	
	return false
end