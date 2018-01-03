Menu.moveable = false;

function Menu.Init(Menu, Data)
	Menu.TitleCon = Gui.HorizontalContainer(Menu, Menu:GetWidth(), 50):SetFocus(false)
	Menu.TitleCon:SetSkin(Gui.GetSkin("Title"))
	Menu.Title = Menu.TitleCon:CreateLabel("Herald")
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.ButList = Gui.VerticalContainer(Menu, 100, 500)
	Menu.ButList:SetSkin(Gui.GetSkin("Big"))
	Menu.ButList:SetY(100)
	Menu.ButList:CreateButton("New",
	function(Widget)
			Gui.SetMenu("GameMenu")
		end):SetWidth(60)
	Menu.ButList:CreateButton("Load",
		function(Widget)
		end):SetWidth(60)
	Menu.ButList:CreateButton("Exit",
		function(Widget) 
			Gui.Close()
		end):SetWidth(60)
	--Menu.ButList:CreateImage(Video.CreateSprite("grass.png"))
	--Menu.ButList:CreateImage(Video.CreateSprite("Grass2.png"))
	--Menu.ButList:CreateTextBox();
	--[[local Stack = Menu:CreateStack(300, 300)

	local a = Stack:AddTab("One")
	local b = a:CreateLabel("Hello")
	Stack:AddTab("Two")
	Stack:AddTab("Three")
	
	Stack:SetX(400)
	Stack:SetY(400)--]]
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
