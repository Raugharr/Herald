Menu.__savestate = false;
Menu.moveable = false;

function Menu.Init(Menu, Data)
	Menu.TitleCon = Gui.HorizontalContainer(Menu, Menu:GetWidth(), 30):SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel("Herald")
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.ButList = Gui.VerticalContainer(Menu, 100, 500)
	Menu.ButList:SetY(100)
	Menu.ButList:CreateButton("New",
	function()
			Gui.SetMenu("GameMenu")
		end)
	Menu.ButList:CreateLabel("Load")
	Menu.ButList:CreateButton("Exit",
		function() 
			Gui.Close()
		end)	
	Menu.ButList:CreateImage(Video.CreateSprite("grass.png"))
	Menu.ButList:CreateImage(Video.CreateSprite("Grass2.png"))
	Menu.ButList:CreateTextBox();
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
