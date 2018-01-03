Menu.moveable = true;
Menu.Width = 500
Menu.Height = 600

function RelationToStr(Guy, Target)
	local Relation = Guy:GetRelation(Target)
	if Relation == nil then
		Relation = "0"
	else
		Relation = tostring(Relation:GetOpinion())
	end
	return Relation
end

function DisplaySettlement(Menu, Left, Right)
	Right:Clear()
	Right:Paragraph("This village currently has " .. tostring(Menu.Settlement:GetPopulation()) .. " people.")
end

function DisplayBigGuys(Menu, Left, Right)
	local Table = nil 
	local Agent = nil
	local Player = World.GetPlayer()
	local Relation = nil
	local Skin = Right:GetSkin()
	local Font = Skin:Table():GetFont()
	
	Right:Clear()
	Table = Right:CreateTable(4, 16, 0, {0, 0, 0, 0})
	Table:CreateLabel("Name")
	Table:CreateLabel("Action");
	Table:CreateLabel("Our Opinion");
	Table:CreateLabel("Their Opinion");
	for Guy in Menu.Settlement:GetBigGuys():Front() do
		if Guy ~= World:GetPlayer() then
			Table:CreateButton(Guy:GetPerson():GetName(),
				function()
					Gui.CreateWindow("ViewPersonMenu", {Person = Guy:GetPerson()})
					Menu:Close()
				end)
			Agent = Guy:GetAgent()
			if Agent ~= nil then
				Table:CreateLabel(Agent:GetAction())
				else
					Table:CreateLabel("Player")
			end
			Table:CreateLabel(RelationToStr(Player, Guy))
			Table:CreateLabel(RelationToStr(Guy, Player))
		end
	end
end

function DisplayCaste(Menu, Settlement)
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Table = nil
	local Count = Settlement:CasteCount()

	Table = Menu:CreateTable(2, #Count + 1)
	
	Table:CreateLabel("Castes")
	Table:CreateLabel("Count")
	Table:CreateLabel("Thrall")
	Table:CreateLabel(Count[1])
	Table:CreateLabel("Serf")
	Table:CreateLabel(Count[2])
	Table:CreateLabel("Freeman")
	Table:CreateLabel(Count[3])
	Table:CreateLabel("Low Noble")
	Table:CreateLabel(Count[4])
	Table:CreateLabel("Noble")
	Table:CreateLabel(Count[5])
	Table:Shrink()
end

function DisplayProfessions(Menu, Settlement)
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Count = Settlement:ProfessionCount()
	local Table = nil

	Table = Menu:CreateTable(2, #Count + 1)
	UpdateProfessions(Table, Settlement)
	return Table
end

function UpdateProfessions(Table, Settlement)
	local Count = Settlement:ProfessionCount()

	Table:DestroyChildren()
	Table:CreateLabel("Professions")
	Table:CreateLabel("Count")
	for i, v in ipairs(Count) do
		if v > 0 then
			Table:CreateLabel(World.Profession(i - 1))
			Table:CreateLabel(v)
		end
	end
	Table:Shrink()
end

function DisplaySelling(Menu, Settlement)
	local Table = Menu:CreateTable(3, 16)	
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()

	Table:SetCellWidth(1, Table:GetCellWidth(1) / 2)
	Table:SetCellWidth(2, Table:GetCellWidth(2) / 2)
	UpdateSelling(Table, Settlement)
	return Table
end

function UpdateSelling(Table, Settlement)
	local Selling = Settlement:Selling()

	Table:DestroyChildren()
	Table:CreateLabel("Selling")
	Table:CreateLabel("Count")
	Table:CreateLabel("Price")
	for Item in Selling:Next() do
		Table:CreateLabel(Item:Good().Name)	
		Table:CreateLabel(Item:Quantity())	
		Table:CreateLabel(Item:Price())	
	end
	Table:Shrink()
end

function DisplayBuying(Menu, Settlement)
	local Buying = Settlement:Buying()
	local Table = Menu:CreateTable(2, 16)	
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()

	UpdateBuying(Table, Settlement)
	return Table
end

function UpdateBuying(Table, Settlement)
	local Buying = Settlement:Buying()

	Table:DestroyChildren()
	Table:CreateLabel("Buying")
	Table:CreateLabel("Count")
	for Item in Buying:Next() do
		Table:CreateLabel(Item:Good().Name)	
		Table:CreateLabel(Item:Quantity())	
	end
	Table:Shrink()
end

function Menu.Init(Menu, Data)
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Main = nil
	local Market = nil
	local Bulletin = nil
	local Settlement = Data.Settlement
	local Government = Settlement:GetGovernment()

	Menu.Settlement = Settlement
	Menu:OnNewChild(Container.Vertical)
	Menu:SetSkin(Gui.GetSkin("Header"))
	Menu:CreateLabel("Settlement Name")
	Menu:SetSkin(Skin)

	Menu.Tabs = Menu:CreateStack(Menu:GetWidth(), Menu:GetHeight() - 100)
	Main = Menu.Tabs:AddTab("Main")
	Main:OnNewChild(Container.Horizontal)

	Menu.Market = Menu.Tabs:AddTab("Market")
	local Market = Menu.Market
	Market:OnNewChild(Container.Horizontal)

	BGStatsContainer(Main, Data.Settlement)
	Main.Gov = Main:CreateTable(2, 5)
	Main.Gov:CreateLabel("Leader")
	Main.Gov:CreateLabel(Government:GetLeader():GetPerson():GetName())
	Main.Gov:CreateLabel("Population")
	Main.Gov:CreateLabel(Settlement:GetPopulation())
	Main.Gov:CreateLabel("Pop Change")
	Main.Gov:CreateLabel(Settlement:YearlyBirths() - Settlement:YearlyDeaths())
	Main.Gov:CreateLabel("Total Acres")
	Main.Gov:CreateLabel("640")
	Main.Gov:CreateLabel("Used Acres")
	Main.Gov:CreateLabel(Data.Settlement:CountAcres())
	Main.Gov:Shrink()
	Main.Acres = Main:CreateTable(2, 2)
	Main.Acres:CreateLabel(Government:Type() .. " " .. Government:Rule())
	Main.Acres:Shrink()
	local WarLabel = Main:CreateLabel("Warriors" .. Settlement:CountWarriors())
	Main:CreateButton("Raise Fyrd",
		function()
			Gui.CreateWindow("ArmyGoal", {Settlement = Settlement})
			Menu:Close()
		end):Below(WarLabel)
	Main:Shrink()

	local Castes = Market:CreateContainer(250, 400)
	Castes:OnNewChild(Container.Vertical)
	DisplayCaste(Castes, Settlement)
	Menu.Professions = DisplayProfessions(Castes, Settlement)
	Castes:Shrink()

	Menu.Selling = DisplaySelling(Market, Settlement)
	Menu.Buying = DisplayBuying(Market, Settlement)
--[[	Market:CreateButton("Bulletin",
		function()
			Gui.CreateWindow("SettlementBulitinMenu", Data)
		end):-]]

	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
end

function Menu.Think(Menu)
	if World.GetDate():Days() == 0 then
		print("Hello World!")
		UpdateSelling(Menu.Selling, Menu.Settlement)
		UpdateBuying(Menu.Buying, Menu.Settlement)
		UpdateProfessions(Menu.Professions, Menu.Settlement)
	end
end

function Menu.Quit(Menu)

end
