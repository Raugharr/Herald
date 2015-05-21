GameMenu = { }

function GameMenu.Init(Width, Height, Data)
	local Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Menu = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0}, Screen)
	local DateCont = GUI.VerticalContainer(0, 0, 512, 100, 0, {0, 0, 0, 0}, Screen)
	
	World.Pause(false)
	DateCont:SetFocus(false)
	Menu:CreateLabel("View Settlement"):OnKey("Enter", "Released",
		function()
			GUI.SetMenu("GovernmentMenu", World.GetSettlement())
		end)
	Menu:CreateLabel("Save")
	GameMenu.Date = DateCont:CreateLabel(PrintDate(World.GetDate()))
	GameMenu.Date:SetFocus(false)
	Menu:CreateLabel("View Self"):OnKey("Enter", "Released",
		function()
			GUI.SetMenu("ViewPersonMenu", World.GetPlayer():GetPerson())
		end)
	Menu:CreateLabel("Advance Time"):OnKey("Enter", "Released",
		function()
			World.Tick()
			GameMenu.Date:SetText(PrintDate(World.GetDate()))
		end)
	Menu:CreateLabel("Main Menu"):OnKey("Enter", "Released",
		function() 
			GUI.PopMenu() 
		end)
	return true
end

function GameMenu.Think(Menu)
	Menu.Date:SetText(PrintDate(World.GetDate()))
end
