Menu.__savestate = false;
Menu.moveable = false;

function Menu.Init(Menu, Data)
	local TempSkin = nil

	Menu.DateCont = Gui.HorizontalContainer(0, 0, 300, 50, Menu):SetFocus(false)
	Menu.MenuBar = Gui.VerticalContainer(0, 0, 512, Menu:GetHeight(), Menu):Below(Menu.DateCont)
	
--	Menu:CreateWorldRender();
	World.Pause(false)
	World.Render(true)
	Menu.MenuBar:CreateButton("Government",
		function()
			Gui.SetMenu("GovernmentMenu", {Settlement = World.GetSettlement()})
		end)
	TempSkin = Menu.DateCont:GetSkin()
	Menu.DateCont:SetSkin(Gui.GetSkin("Header"))
	Menu.Date = Menu.DateCont:CreateLabel(PrintDate(World.GetDate()))
	Menu.Date:SetWidth(100)
	Menu.Popularity = Menu.DateCont:CreateLabel(math.floor(World.GetPlayer():Popularity()) --[[.. "%"--]])
	Menu.Popularity:SetWidth(35)
	Menu.Glory = Menu.DateCont:CreateLabel(math.floor(World.GetPlayer():Glory()) --[[.. "%"--]])
	Menu.Glory:SetWidth(35)
	Menu.Date:SetFocus(false)
	Menu.DateCont:SetX(Menu:GetHorizontalCenter(Menu.DateCont))
	Menu.DateCont:SetSkin(TempSkin)
	Menu.DateCont:Shrink()
	Menu.MenuBar:CreateButton("View Self",
		function()
			Gui.CreateWindow("ViewPersonMenu", {Person = World.GetPlayer():GetPerson()}, 800, 600)
		end)
	Menu.MenuBar:CreateButton("Main Menu",
		function() 
			Gui.PopMenu() 
		end)
	Menu.MenuBar:Shrink()
end

function Menu.Think(Menu)
	Menu.Date:SetText(PrintDate(World.GetDate()))
	Menu.Glory:SetText(World.GetPlayer():Glory())
	Menu.Popularity:SetText(World.GetPlayer():Popularity())
end

function Menu.Quit(Menu)
	World.Pause(true)
	World.Render(false)
end
