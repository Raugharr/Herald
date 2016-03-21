Menu.__savestate = false;
Menu.moveable = false;

function Menu.Init(Menu, Data)
	Menu.DateCont = GUI.HorizontalContainer(0, 0, 512, 50, Menu):SetFocus(false)--(0, 0, 512, 50, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.MenuBar = GUI.VerticalContainer(0, 0, 512, Menu:GetHeight(), Menu):Below(Menu.DateCont)--(0, 0, 512, Height, 0, {0, 0, 0, 0}, Menu.Screen)
	
--	Menu:CreateWorldRender();
	World.Pause(false)
	World.Render(true)
	Menu.MenuBar:CreateButton("View Settlement",
		function()
			GUI.SetMenu("GovernmentMenu", {Settlement = World.GetSettlement()})
		end)
	Menu.Date = Menu.DateCont:CreateLabel(PrintDate(World.GetDate()))
	Menu.Authority = Menu.DateCont:CreateLabel(World.GetPlayer():GetAuthority())
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
end

function Menu.Think(Menu)
	Menu.Date:SetText(PrintDate(World.GetDate()))
	Menu.Authority:SetText(World.GetPlayer():GetAuthority())
end

function Menu.Quit(Menu)
	World.Pause(true)
	World.Render(false)
end
