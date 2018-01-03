Menu.moveable = false;

local function SettlementInfo(Parent)
	local Menu = Gui.VerticalContainer(Parent, 200, 300)

	Menu.Sett = Menu:CreateButton("Settlement",
		function(Widget)
			World.GetPlayer():GetPerson():GetFamily():GetSettlement()
		end)
	Menu.Ruler = Menu:CreateButton("Ruler: ",
		function(Widget)
		end)
	Menu.Gov = Menu:CreateLabel("Government: ",
		function(Widget)
		end)
	Menu.Pop = Menu:CreateLabel("Population: ")
	return Menu
end

local function UpdateSetInfo(Menu, Settlement)
	local Leader = Settlement:GetGovernment():GetLeader()

	Menu.Sett:OnClick(function(Widget)
			Gui.CreateWindow("ViewSettlementMenu", {Settlement = Settlement})
		end)
	Menu.Ruler:SetText("Leader: " .. Leader:GetPerson():GetName())
	Menu.Ruler:OnClick(
		function(Widget)
			Gui.CreateWindow("ViewPersonMenu", {Person = Leader:GetPerson()})
		end)
	Menu.Gov:SetText("Government: " .. Settlement:GetGovernment():GetName())
	Menu.Gov:OnClick(
		function(Widget)
			Gui.CreateWindow("GovernmentMenu", {Settlement = Settlement:GetGovernment()})
			end)
	Menu.Pop:SetText("Population: " .. Settlement:GetPopulation())

end

function Menu.Init(Menu, Data)
	local Player = World.GetPlayer()

	--Menu.MenuBar = Gui.VerticalContainer(Menu, 512, Menu:GetHeight()):Below(Menu.DateCont)--(0, 0, 512, Height, 0, {0, 0, 0, 0}, Menu.Screen)
	
	World.Pause(false)
	World.Render(true)
	Menu.SetInfo = SettlementInfo(Menu) 

	Menu.Left = Gui.HorizontalContainer(Menu, 300, 120)

	Menu.Left.PInfo = Gui.VerticalContainer(Menu.Left, 150, 120)
	Menu.Left.PInfo:CreateButton(Player:GetPerson():GetName(),
		function()
			Gui.CreateWindow("ViewPersonMenu", {Person = World.GetPlayer():GetPerson()})
		end)
	Menu.Left.PInfo:CreateButton(Player:GetFamily():GetSettlement():GetName(),
		function()
			Gui.CreateWindow("GovernmentMenu", {Settlement = World.GetSettlement()})
		end)
	Menu.Popularity = Menu.Left:CreateLabel(math.floor((World.GetPlayer():Popularity() / World.GetPlayer():GetSettlement():CountAdults()) * 100) .. "%")
	Menu.Glory = Menu.Left:CreateLabel(World.GetPlayer():Glory())
	Menu.DateCont = Gui.HorizontalContainer(Menu.Left, 512, 50):SetFocus(false)--(0, 0, 512, 50, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.Date = Menu.DateCont:CreateLabel(PrintDate(World.GetDate()))
	Menu.Date:SetFocus(false)
	--Menu.DateCont:SetX(Menu:GetHorizontalCenter(Menu.DateCont))
	Menu.DateCont:Shrink()
	Menu.SetInfo:SetPos(0, Menu:GetHeight() - Menu.SetInfo:GetHeight())
	Menu.Right = Gui.HorizontalContainer(Menu, 200, 60)
	Menu.Right:CreateButton("Main Menu",
		function() 
			Gui.PopMenu() 
		end)
	Menu.Right:AlignRight()
	--Menu.Right:Shrink()
end

function Menu.Think(Menu)
	Menu.Date:SetText(PrintDate(World.GetDate()))
	Menu.Glory:SetText(World.GetPlayer():Glory())
	Menu.Popularity:SetText(World.GetPlayer():Popularity() .. "%")
end

function Menu.Quit(Menu)
	World.Pause(true)
	World.Render(false)
end

function Menu.Event(Menu, Event)
	if World.Action.Default == Event.Type then
		if Event.Settlement == World.GetPlayer():GetPerson():GetFamily():GetSettlement() then
			Gui.CreateWindow("ViewSettlementMenu", {Settlement = Event.Settlement})
		else
			UpdateSetInfo(Menu.SetInfo, Event.Settlement)
			--Gui.CreateWindow("GovernmentMenu", {Settlement = Event.Settlement:GetGovernment()})
		end
	end
end

