function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0})
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.Title = Menu.TitleCon:CreateLabel("Herald")
	
	Menu.TitleCon:SetFocus(false)
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	
	Menu.Screen:CreateLabel("Raise Fyrd"):OnKey("Enter", "Released",
		function()
			Data["Settlement"]:RaiseArmy()
		end)
	Menu.Screen:CreateLabel("Back"):OnKey("Enter", "Released",
		function()
			GUI.PopMenu()
		end)
	return false
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end