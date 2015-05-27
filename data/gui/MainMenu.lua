MainMenu = { }

function MainMenu.Init(Width, Height, Data)
	MainMenu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {50, 50, 0, 0})
	
	GUI.BackgroundColor(0, 0, 0)
	local TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, MainMenu.Screen)
	TitleCon:SetFocus(false)
	local Title = TitleCon:CreateLabel("Herald")
	Title:SetFocus(false)
	Title:SetX(TitleCon:GetHorizontalCenter(Title))
	MainMenu.Screen:CreateLabel("New"):OnKey("Enter", "Released",
		function()
			GUI.SetMenu("GameMenu")
		end)
	MainMenu.Screen:CreateLabel("Load")
	MainMenu.Screen:CreateLabel("Exit"):OnKey("Enter", "Released", 
		function() 
			GUI.PopMenu()
		end)	
	return false
end

function MainMenu:Think(Menu)

end

function MainMenu:Quit(Menu)

end
