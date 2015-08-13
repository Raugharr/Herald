function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	Menu.MenuBar = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.DateCont = GUI.VerticalContainer(0, 0, 512, 100, 0, {0, 0, 0, 0}, Menu.Screen)
	
	World.Pause(false)
	World.Render(true)
	Menu.DateCont:SetFocus(false)
	Menu.MenuBar:CreateButton("View Settlement",
		function()
			GUI.SetMenu("GovernmentMenu", {Settlement = World.GetSettlement()})
		end)
	Menu.Date = Menu.DateCont:CreateLabel(PrintDate(World.GetDate()))
	Menu.Date:SetFocus(false)
	Menu.DateCont:Shrink()
	Menu.MenuBar:CreateButton("View Self",
		function()
			GUI.SetMenu("ViewPersonMenu", World.GetPlayer():GetPerson())
		end)
	Menu.MenuBar:CreateButton("Main Menu",
		function() 
			GUI.PopMenu() 
		end)
	Menu.MenuBar:Shrink()
	return false
end

function Menu.Think(Menu)
	Menu.Date:SetText(PrintDate(World.GetDate()))
end

function Menu.Quit(Menu)
	World.Pause(true)
	World.Render(false)
end
