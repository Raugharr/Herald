MainMenu = { }

function MainMenu.Init(Width, Height)
	MainMenu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 2, {50, 50, 0, 0})
	
	GUI.BackgroundColor(0, 0, 0)
	MainMenu.Screen:CreateTextBox("New")
	MainMenu.Screen:CreateTextBox("Load")
	MainMenu.Screen:CreateTextBox("Debug"):OnKey("Enter", "Released", 
	function() 
		GUI.SetMenu("DebugMenu") 
	end)
	MainMenu.Screen:CreateTextBox("Exit"):OnKey("Enter", "Released", 
	function() 
		MainMenu.Screen:Close()
	end)
	return false
end