Menu.__savestate = false;
Menu.moveable = false;

Strings = {
	"Popularity measures how well respected you are by the people.",
	"Glory represents how respected you are by the warrior caste."
}

function InfoWidgetClose(Widget)
	Widget.Window:Close()
end

function Menu.Init(Menu, Data)
	local TempSkin = nil

	Menu.DateCont = Gui.HorizontalContainer(Menu, 300, 50):SetFocus(false)
	Menu.MenuBar = Gui.VerticalContainer(Menu, 512, Menu:GetHeight()):Below(Menu.DateCont)
	Menu.HoverCont = nil
	
--	Menu:CreateWorldRender();
	World.Pause(false)
	World.Render(true)
	Menu.MenuBar:CreateButton("Government",
		function(Widget)
			Gui.CreateWindow("GovernmentMenu", {Settlement = World.GetSettlement()})
		end)
	Menu.MenuBar:CreateButton("Faction",
		function(Widget)
			Gui.CreateWindow("FactionMenu", {Player = World.GetPlayer()})
		end)	
	TempSkin = Menu.DateCont:GetSkin()
	Menu.DateCont:SetSkin(Gui.GetSkin("Header"))
	Menu.Date = Menu.DateCont:CreateLabel(PrintDate(World.GetDate()))
	Menu.Date:OnClick(
		function(Widget)
			if World.IsPaused() == true then
				World.Pause(false)			
			else
				World.Pause(true)
			end
	--		Gui.CreateWindow("GameHover", {Settlement = nil}, 120, 120)
		end)
	--Menu:OnHover(function(Menu)
	--Menu:OnClick(
	--	function()
			--local x, y = GetMousePos()
			--local Settlement = World.GetSettlement(World.GetMapPos(x, y))
			
			--if Settlement ~= nil then
				--Menu.HoverCont = Gui.CreateWindow("MessageBox", {Text = "Pop:"}, 120, 120)
			--end
	--	end)
	--[[Menu:OnHoverLoss(function(Menu)
		if Menu.HoverCont ~= nil then
			Menu.HoverCont:Close()
			Menu.HoverCont = nil
		end
		end)--]]
	Menu.Date:SetWidth(100)
	Menu.Popularity = Menu.DateCont:CreateLabel(math.floor(World.GetPlayer():Popularity()))
	Menu.Popularity:SetWidth(35)
	Menu.Popularity:OnHover(
		function(Widget)
			Widget.Window = CreateInfoWindow(Strings[1])
		end)
	Menu.Popularity:OnHoverLoss(InfoWidgetClose)
	Menu.Glory = Menu.DateCont:CreateLabel(math.floor(World.GetPlayer():Glory()))
	Menu.Glory:SetWidth(35)
	Menu.Glory:OnHover(
		function(Widget)
			Widget.Window = CreateInfoWindow(Strings[2])
		end)
	Menu.Glory:OnHoverLoss(InfoWidgetClose)
	Menu.Date:SetFocus(false)
	Menu.DateCont:SetX(Menu:GetHorizontalCenter(Menu.DateCont))
	Menu.DateCont:SetSkin(TempSkin)
	Menu.DateCont:Shrink()
	Menu.MenuBar:CreateButton("View Self",
		function(Widget)
			Gui.CreateWindow("ViewPersonMenu", {Person = World.GetPlayer():GetPerson()})
		end)
	Menu.MenuBar:CreateButton("Plots",
		function(Widget)
			Gui.CreateWindow("PlotMenu", {Plot = World.GetPlot(World.GetPlayer())})
		end)
	Menu.MenuBar:CreateButton("Main Menu",
		function(Widget) 
			Gui.PopMenu() 
		end)
	Menu.MenuBar:Shrink()
end

function Menu.Think(Menu)
	Menu.Date:SetText(PrintDate(World.GetDate()))
	Menu.Glory:SetText(World.GetPlayer():Glory())
	Menu.Popularity:SetText(World.GetPlayer():Popularity())
end

--[[function Menu.OnKey(Menu, KeyState)
	if KeyState:Key('-') == true then
		World.SetSpeed(World.GetSpeed() - 1)
	else if KeyState:Key('+') == true then
		World.SetSpeed(World.GetSpeed() + 1)
	end
end--]]

function Menu.Quit(Menu)
	World.Pause(true)
	World.Render(false)
end
