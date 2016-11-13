Menu.__savestate = false;
Menu.moveable = true;

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
	
	Right:Clear()
	Table = Right:CreateTable(4, 16, 0, {0, 0, 0, 0})
	Table:SetCellWidth(Gui.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(Gui.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name")
	Table:CreateLabel("Action");
	Table:CreateLabel("Our Opinion");
	Table:CreateLabel("Their Opinion");
	for Guy in Menu.Settlement:GetBigGuys():Front() do
		if Guy ~= World:GetPlayer() then
			Table:CreateButton(Guy:GetPerson():GetName(),
				function()
					Gui.SetMenu("ViewPersonMenu", {Person = Guy:GetPerson()})
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

function Menu.Init(Menu, Data)
	Menu.TitleCon = Gui.HorizontalContainer(0, 0, Menu:GetWidth(), 30, Menu)
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
			Gui.CreateWindow("SettlementBulitinMenu", Data, 512, 512)
		end)
	Menu.Left:CreateButton("Close",
		function()
			Menu:Close()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
