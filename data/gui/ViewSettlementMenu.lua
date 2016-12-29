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
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
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

	Table = Menu:CreateTable(2, 5)
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	
	Table:CreateLabel("Thrall")
	Table:CreateLabel("0")
	Table:CreateLabel("Serf")
	Table:CreateLabel("0")
	Table:CreateLabel("Freeman")
	Table:CreateLabel("0")
	Table:CreateLabel("Warrior")
	Table:CreateLabel("0")
	Table:CreateLabel("Noble")
	Table:CreateLabel("0")
	Table:Shrink()
end

function Menu.Init(Menu, Data)
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Main = nil
	local Settlement = Data.Settlement
	local Government = Settlement:GetGovernment()

	Menu:OnNewChild(Container.Vertical)
	Menu:SetSkin(Gui.GetSkin("Header"))
	Menu:CreateLabel("Settlement Name")
	Menu:SetSkin(Skin)
	Main = Gui.HorizontalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	BGStatsContainer(Main, Data.Settlement)
	Main.Gov = Main:CreateTable(2, 5)
	Main.Gov:SetCellWidth(Font:Width() * 8)
	Main.Gov:SetCellHeight(Font:Height())
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
	Main.Acres:SetCellWidth(Font:Width() * 8)
	Main.Acres:SetCellHeight(Font:Height())
	Main.Acres:CreateLabel(Government:Type() .. " " .. Government:Rule())
	Main.Acres:Shrink()
	Main:CreateButton("Raise Fyrd",
		function()
			World.SetOnClick(1, Settlement.__self)
			Menu:Close()
		end)

	Main:Shrink()
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
--[[	Menu.TitleCon = Gui.HorizontalContainer(0, 0, Menu:GetWidth(), 30, Menu)
	Menu.Left = Gui.VerticalContainer(0, 30, (Menu:GetWidth() / 2), (Menu:GetHeight() - 30), Menu)
	Menu.Right = Gui.VerticalContainer((Menu:GetWidth() / 2), 30, (Menu:GetWidth() / 2), (Menu:GetHeight() - 30), Menu)
	Menu.Title = Menu.TitleCon:CreateLabel("View Settlement")
	Menu.Settlement = Data["Settlement"]
	
	Menu.TitleCon:SetFocus(false)
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	
	Menu.Left:CreateLabel(tostring(Data["Settlement"]:CountAcres()) .. " Acres")
	Menu.Left:Paragraph("There are " .. Menu.Settlement:GetFreeWarriors() .. " warriors to recruit out of " .. Menu.Settlement:GetMaxWarriors())
	Menu.Left:Paragraph("The settlement can currently can feed " .. tostring(math.floor(Data["Settlement"]:GetNutrition() / 365 / Person.DailyNut)) .. " people a year." 
		.. " we expect to be able to feed " .. tostring(math.floor(Data["Settlement"]:ExpectedYield() / 365 / Person.DailyNut)) .. " people from our harvest this year." )
	Menu.Left:Paragraph(tostring(Data["Settlement"]:YearlyDeaths()) .. " people have died and " .. tostring(Data["Settlement"]:YearlyBirths()) .. " children have been born this year.")
	
	Menu.Left:CreateButton("Raise Fyrd",
		function()
			World.SetOnClick(1, Menu.Settlement.__self)
			Menu:Close()
		end)
	Menu.Left:CreateButton("View Settlement",
		function()
			DisplaySettlement(Menu, Menu.Left, Menu.Right)
		end)
	Menu.Left:CreateButton("View big guys",
		function()
			DisplayBigGuys(Menu, Menu.Left, Menu.Right)
		end)
	Menu.Left:CreateButton("Bulitin",
		function()
			Gui.CreateWindow("SettlementBulitinMenu", Data)
		end)
	Menu.Left:CreateButton("Close",
		function()
			Menu:Close()
		end)
	--]]
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
