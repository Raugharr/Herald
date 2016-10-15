Menu.__savestate = false
Menu.moveable = true

require("BigGuyAux")
require("PlotMenu")

--[[function DisplayPlotMenu(Menu, Left, Right)
	local Actions = {}
	local Plot = Menu.Plot

	Left:Clear()
	if Plot:IsStarted() == false then
		Left:CreateButton("Start plot.",
			function()
				Plot:Start(true)
			end)
	end
	Left:CreateLabel("Current threat: " .. tostring(Plot:GetThreat()))
	Left:CreateLabel("Warscore: " .. tostring(Plot:GetScore()))
	Left:CreateButton("Show Plotters", 
		function()
			DisplayPlotters(Menu, Left, Right)
		end)
	Left:CreateButton("Show Defenders", 
		function()
			DisplayPlotDefenders(Menu, Left, Right)
		end)
	Left:CreateButton("Show History",
		function()
			Right:Clear()
			for i, Action in ipairs(Plot:PrevMonthActions()) do
				Right:CreateLabel(Action:Describe())	
			end
		end)
	Menu.CbtLbl = Left:CreateLabel("Combat Abilities")
	Actions[#Actions + 1] = Left:CreateButton("Double Damage",
		function()
			Plot:AddAction(Plot.DoubleDamage,  World.GetPlayer(), nil)			
		end)
	Actions[#Actions + 1] = Left:CreateButton("Double Attack",
		function()
			Plot:AddAction(Plot.DoubleAttack,  World.GetPlayer(), nil)			
		end)
	Left:CreateLabel("Wit Abilities")
	Actions[#Actions + 1] = Left:CreateButton("Prevent Damage",
		function()
			Plot:AddAction(Plot.Prevent,  World.GetPlayer(), Menu.Guy)			
		end)
	Left:CreateLabel("Charisma")
	Left:CreateButton("Back", function()
		DisplayViewPerson(Menu, Left, Right)
	end)
end
--]]

function DisplayPlots(Menu, Left, Right)
	local Guy = Menu.Guy
	local Retinue = Guy:GetPerson():Retinue()

	Right:Clear()
	if Retinue ~= nil and Retinue:Leader():Equal(Guy) then
		Right:CreateButton("Takeover Retinue",
			function()
			Plot.Create(World.GetPlayer(), Guy, Plot.Type.ControlRetinue)
		end)
	end
	Right:CreateButton("Back", 
		function()
			Right:Clear()
		end)
end

function NonPlayerActions(Menu, Left, Right)
	local Person = Menu.Guy

	GeneralActions(Right, World.GetPlayer(), Person)
	Right:CreateButton("Back",
		function()
			Right:Clear()
			DisplayViewPerson(Menu, Left, Right)
		end)
end

function DisplayPlotsAgainst(Menu, Left, Right)
	Left:Clear()
	for Elem in Menu.Guy:PlotsAgainst():Front() do
		Left:CreateButton(Elem:TypeStr(),
			function()
				Menu.Plot = Elem
				DisplayPlotMenu(Menu, Left, Right)
			end)
	end
	Left:CreateButton("Back", 
		function()
			DisplayViewPerson(Menu, Left, Right)
		end)
end

function DisplayManageHousehold(Menu, Left, Right)
	Left:CreateButton("Agriculture",
		function()
			if(Right~= nil) then
				Right:Destroy()
			end
			Right = Left:CreateTable(5, 16)	
			Right:SetX(401)
			Right:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Right:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			
			Right:CreateLabel("Name"):SetFocus(false)
			Right:CreateLabel("Yield"):SetFocus(false)
			Right:CreateLabel("Acres"):SetFocus(false)
			Right:CreateLabel("Status"):SetFocus(false)
			Right:CreateLabel("StatusTime"):SetFocus(false)
			for k, Field in ipairs(Menu.Person:GetFamily():GetFields()) do	
				Right:CreateLabel(Field:GetCrop().Name)
				Right:CreateLabel(Field:GetYield())
				Right:CreateLabel(Field:GetAcres())
				Right:CreateLabel(Field:GetStatus())
				Right:CreateLabel(tostring(Field:StatusCompletion()) .. " days")
			end
		end)
	Left:CreateButton("Goods",
		function()
			if(Right ~= nil) then
				Right:Destroy()
			end
			Right = Left:CreateTable(2, 16)
			Right:SetX(401)
			Right:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Right:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			Right:CreateLabel("Name"):SetFocus(false)
			Right:CreateLabel("Quantity"):SetFocus(false)
			for val in Menu.Person:GetFamily():GetGoods():Next() do
				Right:CreateLabel(val:GetBase().Name)
				Right:CreateLabel(val:GetQuantity())
			end
		end)
	Left:CreateButton("Animals",
		function()
			Right:Clear()
			Table = CreateAnimalTable(Right, 16)
			for An in Menu.Person:GetFamily():GetAnimals():Next() do
				Table:CreateLabel(An:GetBase().Name)
				Table:CreateLabel(An:GetNutrition())
				Table:CreateLabel(PrintYears(An:GetAge()))
				Table:CreateLabel(GenderName(An))
			end
		end)
	Left:CreateButton("Buildings",
	 function()
		if(Right ~= nil) then
			Right:Destroy()
		end
		Right = Left:CreateTable(3, 16)
		Right:SetX(401)
		Right:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
		Right:SetCellHeight(GUI.GetDefaultFont():FontHeight())
		Right:CreateLabel("Width"):SetFocus(false)
		Right:CreateLabel("Length"):SetFocus(false)
	end)
	Left:CreateButton("Back",
		function()
			Left:Clear()
			if(Right ~= nil) then
				Right:Clear()
			end
		DisplayViewPerson(Menu, Left, Right)
	end)
end


function DisplayPersonStats(Menu, Left, Right)
	local Person = Menu.Guy

	Right:AddChild(BGStatsContainer(Menu.Guy))
end

function DisplayFriends(Menu, Left, Right)
	Right:Clear()

	local Table = CreatePersonTable(Right, 8)
	local List = {}
	local Idx = 1

	for Rel in Menu.Guy:RelationsItr() do
		if(Rel:GetOpinion() > 25) then
			List[Idx] = Rel:BigGuy():GetPerson()
			Idx = Idx + 1
		end
	end
	FillPersonTable(Table, List)
end

function DisplayEnemies(Menu, Left, Right)
	Right:Clear()

	local Table = CreatePersonTable(Right, 8)
	local List = {}
	local Idx = 1

	for Rel in Menu.Guy:RelationsItr() do
		if(Rel:GetOpinion() < -25) then
			List[Idx] = Rel:BigGuy():GetPerson()
			Idx = Idx + 1
		end
	end
	FillPersonTable(Table, List)
end

function DisplayFamily(Menu, Left, Right)
	local Table = nil
	local PersonTable = nil
	local Guy = Menu.Guy

	Right:Clear()
	PersonTable = Menu.Person:GetFamily():GetPeople() 
	FillPersonTable(CreatePersonTable(Right, #PersonTable), PersonTable)
end

function DisplayRecruitStats(Menu, Left, Right)
	local Guy = Menu.Guy	
	local Person = Guy:GetPerson()
	local Retinue = Person:Retinue()
	local Warriors = nil 
	if Retinue == nil then
		return	
	end
	Warriors = Retinue:Warriors()
	Right:Clear()
	Right:Paragraph("You currently have " .. Warriors:GetSize() .. " warriors in your retinue.")
	Right:Paragraph("The leader of the retinue is " .. Retinue:Leader():GetPerson():GetName() .. " and has " .. Retinue:Leader():Glory() .. " glory.")
	FillWarriorTable(CreateWarriorTable(Right, Warriors:GetSize()), Warriors)
end

function DisplayRelation(Menu, Left, Right)
	local Guy = Menu.Guy
	local Relation = World:GetPlayer():GetRelation(Guy)
	local List = Relation:GetRelationList()

	Right:Clear()
	FillRelationList(CreateRelationTable(Right, #List), List)
end

function DisplayViewPerson(Menu, Left, Right)
	local Guy = Menu.Guy

	Left:Clear()
	Right:Clear()
	Left:Paragraph(Menu.Description)
	Left:CreateLabel("Owns " .. Menu.OxCount .. " cows.")
	Left:CreateLabel("Skills")
	Left:CreateButton("Household",
		function()
			Left:Clear()
			DisplayManageHousehold(Menu, Left, Right)
		end
	)
	--FIXME: change class comparason to comapre their __self parameters insead of checking the table address.
	if World.GetPlayer() ~= Guy then
		Left:CreateButton("Actions",
			function()
				Right:Clear()
				NonPlayerActions(Menu, nil, Right)
			end)
		Left:CreateButton("Retinue",
			function()
				DisplayRecruitStats(Menu, Left, Right)
			end)
	end

	Left:CreateButton("Stats", 
		function()
			Right:Clear()
			DisplayPersonStats(Menu, Left, Right)
		end)
	Left:CreateButton("Family",
		function()
			DisplayFamily(Menu, Left, Right)
		end)
	Left:CreateButton("Friends",
		function()
			DisplayFriends(Menu, Left, Right)
		end)
	Left:CreateButton("Enemies",
		function()
			DisplayEnemies(Menu, Left, Right)
		end)
	if World:GetPlayer() ~= Menu.Guy then
		Left:CreateButton("View Relation",
			function()
				DisplayRelation(Menu, Left, Right)
			end)
	end
	if World.GetPlot(Guy) ~= nil then
		Menu.Plot = World.GetPlot(Guy)
		Left:CreateButton("Plots", 
			function()
				DisplayPlotMenu(Menu, Left, Right)
			end)
	else
		Left:CreateButton("Plots",
			function()
				DisplayPlots(Menu, Left, Right)
			end)
	end 
	Left:CreateButton("Plots Against",
		function()
			DisplayPlotsAgainst(Menu, Left, Right)
		end)
	Left:CreateButton("Back",
		function()
			GUI.PopMenu()
		end)
end

function Menu.Init(Menu, Person)
	local Guy = World.GetBigGuy(Person.Person)
	local Family = Person.Person:GetFamily()

	Menu.Person = Person.Person
	Menu.MenuBar = GUI.VerticalContainer(0, 0, 400, Menu:GetHeight(), Menu)
	Menu.Display = GUI.VerticalContainer(401, 0, Menu:GetWidth(), Menu:GetHeight(), Menu) 
	Menu.Description = Guy:GetName() .. " is of the family " .. Family:GetName() .. ". "
		 .. "He owns " .. Family:GetBuildingCt() .. " buildings and " .. Family:GetAnimalCt() .. " animals."
		.. "This family can currently feed itself for "
		 .. tostring(math.floor(Family:GetNutrition() / Family:GetNutritionReq())) .. " days"
	Menu.OxCount = Family:CountAnimal("Ox")
	
	Menu.Guy = Guy
	if World.GetPlayer():GetPerson() ~= Person and Guy ~= nil then
		local Rel = Guy:GetRelation(World.GetPlayer())
		if Rel == nil then
			Rel = 0
		else
			Rel = Rel:GetOpinion()
		end
		Menu.Description = Menu.Description.. " Their opinion of us is " .. Rel
	end
	DisplayViewPerson(Menu, Menu.MenuBar, Menu.Display)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
