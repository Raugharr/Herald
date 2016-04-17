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

function Menu.Init(Menu, Data)
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Menu:GetWidth(), 30, Menu)
	Menu.Left = GUI.VerticalContainer(0, 30, (Menu:GetWidth() / 2), (Menu:GetHeight() - 30), Menu)
	Menu.Right = GUI.VerticalContainer((Menu:GetWidth() / 2), 30, (Menu:GetWidth() / 2), (Menu:GetHeight() - 30), Menu)
	Menu.Title = Menu.TitleCon:CreateLabel("View Settlement")
	local BigGuyMenu = nil
	local SettlementMenu = nil
	
	Menu.TitleCon:SetFocus(false)
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	
	Menu.Left:CreateLabel(tostring(Data["Settlement"]:CountAcres()) .. " Acres")
	Menu.Left:Paragraph("The settlement can currently can feed " .. tostring(math.floor(Data["Settlement"]:GetNutrition() / 365 / 8)) .. " people a year." 
		.. " we expect to be able to feed " .. tostring(math.floor(Data["Settlement"]:ExpectedYield() / 365 / 8)) .. " people from our harvest this year." )
	Menu.Left:Paragraph(tostring(Data["Settlement"]:YearlyDeaths()) .. " people have died and " .. tostring(Data["Settlement"]:YearlyBirths()) .. " children have been born this year.")
	
	Menu.Left:CreateButton("Raise Fyrd",
		function()
			World.SetOnClick(1, Data["Settlement"].__self)
			--Data["Settlement"]:RaiseArmy()
			Gui.MenuPop()
		end)
	Menu.Left:CreateButton("View Settlement",
		function()
			if SettlementMenu ~= nil then
				return
			end
			SettlementMenu = Menu.Right:CreateLabel(tostring(Data["Settlement"]:GetPopulation()))
		end)
	Menu.Left:CreateButton("View big guys",
		function()
			if BigGuyMenu ~= nil then
				return
			end

			local Table = Menu.Right:CreateTable(4, 16, 0, {0, 0, 0, 0})
			local Agent = nil
			local Player = World.GetPlayer()
			local Relation = nil
			
			BigGuyMenu = Table
			Table:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Table:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			Table:CreateLabel("Name")
			Table:CreateLabel("Action");
			Table:CreateLabel("Our Opinion");
			Table:CreateLabel("Their Opinion");
			for Guy in Data["Settlement"]:GetBigGuys():Front() do
				if Guy:Equal(World:GetPlayer()) == false then
					Table:CreateButton(Guy:GetPerson():GetName(),
						function()
							GUI.SetMenu("ViewPersonMenu", Guy:GetPerson())
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
		end)
	Menu.Left:CreateButton("Bulitin",
		function()
			GUI.CreateWindow("SettlementBulitinMenu", Data, 512, 512)
		end)
	Menu.Left:CreateButton("Close",
		function()
			GUI.PopMenu()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
