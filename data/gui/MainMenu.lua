function PrintTable(Table)
	for k, v in pairs(Table) do
		print(k .. "\n")
	end
end

MainMenu = { }

function MainMenu.Init(Width, Height)
	local Screen = GUI.VerticalContainer(0, 0, Width, Height)
	
	GUI.BackgroundColor(0, 0, 0)
	Screen:CreateTextBox("New")
	Screen:CreateTextBox("Load")
	Screen:CreateTextBox("Debug"):OnKey("Enter", "Down", 
	function() 
		GUI.SetMenu("DebugMenu") 
	end)
	Screen:CreateTextBox("Exit"):OnKey("Enter", "Down", 
	function() 
		GUI.Quit()
	end)
end