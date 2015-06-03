function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {50, 50, 0, 0})
	
	GUI.BackgroundColor(0, 0, 0)
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel("Herald")
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.Screen:CreateLabel("New"):OnKey("Enter", "Released",
		function()
			GUI.SetMenu("GameMenu")
		end)
	Menu.Screen:CreateLabel("Load")
	Menu.Screen:CreateLabel("Exit"):OnKey("Enter", "Released", 
		function() 
			GUI.PopMenu()
		end)	
	return false
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
