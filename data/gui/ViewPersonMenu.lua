Menu.__savestate = false;
Menu.moveable = true;

require("BigGuyAux")

function NonPlayerActions(Menu, Left, Right)
	local Person = Menu.Guy

	Right:CreateButton("Influence", 
		function()
			World.GetPlayer():SetAction(BigGuy.Action.Influence, Person)
		end)
	Right:CreateButton("Sabotage",
		function()
			World.GetPlayer():SetAction(BigGuy.Action.Sabotage, Person)
		end)
	Right:CreateButton("Duel",
		function()
			World.GetPlayer():SetAction(BigGuy.Action.Duel, Person)
		end)
	Right:CreateButton("Steal Cattle",
		function()
			World.GetPlayer():SetAction(BigGuy.Action.StealCattle, Person)
		end)
	Right:CreateButton("Murder",
		function()
			World.GetPlayer():SetAction(BigGuy.Action.Murder, Person)
		end)
	Right:CreateButton("Cause Dissent",
		function()
			World.GetPlayer():SetAction(BigGuy.Action.Dissent, Person)
		end)
	Right:CreateButton("Convince",
		function()
			World.GetPlayer():SetAction(BigGuy.Action.Convince, Person)
		end)
	Right:CreateButton("Back",
		function()
			Right:Clear()
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
			for Field in Person:GetFamily():GetFields():Next() do	
				Right:CreateLabel(Field:GetCrop().Name)
				Right:CreateLabel(Field:GetYield())
				Right:CreateLabel(Field:GetAcres())
				Right:CreateLabel(Field:GetStatus())
				Right:CreateLabel(Field:GetStatusTime())
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
			for val in Person:GetFamily():GetGoods():Next() do
				Right:CreateLabel(val:GetBase().Name)
				Right:CreateLabel(val:GetQuantity())
			end
		end)
	Left:CreateButton("Animals",
		function()
			if(Right ~= nil) then
				Right:Destroy()
			end
			Right = Left:CreateTable(3, 16)
			Right:SetX(401)
			Right:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Right:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			Right:CreateLabel("Name"):SetFocus(false)
			Right:CreateLabel("Nutrition"):SetFocus(false)
			Right:CreateLabel("Age"):SetFocus(false)
			for An in Person:GetFamily():GetAnimals():Next() do
				Right:CreateLabel(An:GetBase().Name)
				Right:CreateLabel(An:GetNutrition())
				Right:CreateLabel(PrintYears(An:GetAge()))
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
	local Person = Menu.Guy;

	Right:AddChild(BGStatsContainer(Menu.Guy))
end

function DisplayFriends(Menu, Left, Right)
	Right:Clear()

	local Table = CreatePersonTable(Right, 8)
	local List = { }
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
	local List = { }
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
	PersonTable = Guy:GetFamily():GetPeople() 
	FillPersonTable(CreatePersonTable(Right, #PersonTable), PersonTable)
end

function DisplayViewPerson(Menu, Left, Right)
	local Guy = Menu.Guy

	Left:Paragraph(Menu.Description)
	Left:CreateLabel("Owns " .. Menu.OxCount .. " cows.")
	Left:CreateLabel("Skills")
	Left:CreateButton("Manage household",
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
	if World.GetPlot(Guy) ~= nil then
		Left:CreateButton("Plots", 
			function()
				GUI.SetMenu("PlotMenu", {Person = Guy, Plot = World.GetPlot(Guy)}) 
			end)
	else
		Left:CreateLabel("Plots")
	end 
	Left:CreateButton("Back",
		function()
			GUI.PopMenu()
		end)
end

function Menu.Init(Menu, Person)
	Menu.MenuBar = GUI.VerticalContainer(0, 0, 400, Menu:GetHeight(), Menu)
	Menu.Display = GUI.VerticalContainer(401, 0, Menu:GetWidth(), Menu:GetHeight(), Menu) 
	Menu.Description = Person:GetName() .. " is of the family " .. Person:GetFamily():GetName() .. ". " .. "He owns " .. Person:GetFamily():GetBuildingCt() .. " buildings and " .. Person:GetFamily():GetAnimalCt() .. " animals."
	Menu.OxCount = Person:GetFamily():CountAnimal("Ox")
	local Guy = nil
	
	Guy = World.GetBigGuy(Person)
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
