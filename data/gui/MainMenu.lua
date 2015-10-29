Menu.__savestate = false;
Menu.moveable = false;

function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	
	GUI.BackgroundColor(0, 0, 0)
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel("Herald")
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.Screen:CreateButton("New",
	function()
			GUI.SetMenu("GameMenu")
		end)
	Menu.Screen:CreateLabel("Load")
	Menu.Screen:CreateButton("Exit",
		function() 
			Menu.Screen:Close()
		end)	
	Menu.Screen:CreateImage(Video.CreateSprite("grass.png"))
	Menu.Screen:CreateImage(Video.CreateSprite("Grass2.png"))
	return Menu.Screen
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
