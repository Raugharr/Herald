Menu.moveable = true
Menu.Width = 500
Menu.Height = 600

require("BigGuyAux")

local function CombatAbilities(Menu)
	local Cont = Gui.VerticalContainer(Menu, 100, 100)

	Cont:CreateButton("Double Damage",
		function(Widget)
			Plot:AddAction(Plot.DoubleDamage,  World.GetPlayer(), nil)			
		end)
	Cont:CreateButton("Double Attack",
		function(Widget)
			Plot:AddAction(Plot.DoubleAttack,  World.GetPlayer(), nil)			
		end)
	return Cont
end


local function WitAbilities(Menu)
	local Cont = Gui.VerticalContainer(Menu, 100, 100)

	Menu:CreateButton("Prevent Damage",
		function()
			Plot:AddAction(Plot.Prevent,  World.GetPlayer(), Menu.Guy)			
		end)
	return Cont
end

local function IntrigueAbilities(Menu)
	local Cont = Gui.VerticalContainer(Menu, 100, 100)

	Menu :CreateButton("Prevent Damage",
		function(Widget)
		end)
	return Cont
end

function Menu.Init(Menu, Data)
	local Actions = {}
	local Plot = Data["Plot"] 
	local Skin = Menu:GetSkin()
	local Header = Gui.GetSkin("Header")
	local AttackThreat, DefenderThreat = Plot:GetThreat()

	Menu:OnNewChild(Container.Vertical)
	Menu:CreateLabel("Plot to " .. "overthrow " .. Plot:Target():GetPerson():GetName())
	Menu:CreateLabel("Warscore: " .. tostring(Plot:GetScore()))
	if Plot:HasStarted() == false then
		Menu.PlotButton = Menu:CreateButton("Start plot",
			function(Widget)
				Plot:Start(true)
				
			end)
	else
		Menu.PlotButton = Menu:CreateButton("Abandon plot",
			function(Widget)
			end)	
	end
	Menu.People = Gui.HorizontalContainer(Menu, Menu:GetWidth(), 200)
	Menu.Plotters = DisplayPlotSide(Menu.People, Plot, "Attackers", AttackThreat, true)
	Menu.Defenders = DisplayPlotSide(Menu.People, Plot, "Defenders", DefenderThreat, false)
	--Menu.AbilitySelect = Gui.VerticalContainer(Menu, 400, 60)
	Menu.History = Gui.VerticalContainer(Menu, Menu:GetWidth(), 250)
	Menu.History:SetSkin(Header)
	Menu.History:CreateLabel("Plot History")
	Menu.History:SetSkin(Skin)
	for i, Action in ipairs(Plot:PrevMonthActions()) do
		Menu.History:CreateLabel(Action:Describe())	
	end
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
	--[[Menu.AbilitySelect.Buttons = Gui.HorizontalContainer(Menu.AbilitySelect, Menu.AbilitySelect:GetWidth(), 20)
	Menu.AbilitySelect.Buttons:CreateButton("Combat Abilities",
		function(Widget)
			Menu.Abilities:Destroy()
			Menu.Abilities = CombatAbilities(Menu)
		end)
	Menu.AbilitySelect.Buttons:CreateButton("Wit Abilities",
		function(Widget)
			Menu.Abilities:Destroy()
			Menu.Abilities = WitAbilities(Menu)
		end)
	Menu.AbilitySelect.Buttons:CreateButton("Charisma Abilities",
		function(Widget)
			Menu.Abilities:Destroy()
			Menu.Abilities = CharismaAbilities(Menu)
		end)
	Menu.Abilities = CombatAbilities(Menu.AbilitySelect)
	--]]
end

function DisplayPlotSide(Menu, Plot, Title, Threat, Plotter)
	local List = {}
	local PersonList = {} 
	local Skin = Menu:GetSkin()
	local Header = Gui.GetSkin("Header")
	local Container = Gui.VerticalContainer(Menu, Menu:GetWidth() / 2, 200)
	local Font = Skin:Table():GetFont()
	local Table = nil	

	Container:SetSkin(Header)
	Container:CreateLabel(Title)
	Container:CreateLabel("Threat " .. Threat)
	Container:SetSkin(Skin)
	Table = Container:CreateTable(1, 8)
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	if Plotter == true then
		for v in Plot:Plotters():Front() do
			Table:CreateLabel(v:GetPerson():GetName())
		end
		else 
		for v in Plot:Defenders():Front() do
			Table:CreateLabel(v:GetPerson():GetName())
		end
	end
	return Container
end

function DisplayPlotters(Menu, Plot)
	local List = {}
	local PersonList = {} 
	local Skin = Menu:GetSkin()
	local Header = Gui.GetSkin("Header")
	local Container = Gui.VerticalContainer(Menu, 100, Menu:GetWidth() / 2)

	Container:SetSkin(Header)
	Container:CreateLabel("Plotters")
	Container:SetSkin(Skin)
	List = FillList(Plot:Plotters().Front, Plot:Plotters())
	for k, v in ipairs(List) do
		PersonList[#PersonList + 1] = v:GetPerson()
	end
	FillPersonTable(CreatePersonTable(Container, #PersonList), PersonList, Plot:Plotters())
	return Container
end

