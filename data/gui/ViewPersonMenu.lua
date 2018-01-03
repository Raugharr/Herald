Menu.moveable = true
Menu.Width = 500
Menu.Height = 650

require("BigGuyAux")

function DisplayPlots(Menu, Owner)
	local Retinue = Owner:GetPerson():Retinue()

	if Retinue ~= nil and Retinue:Leader():Equal(Owner) then
		Menu:CreateButton("Takeover Retinue",
			function()
			Plot.Create(World.GetPlayer(), Owner, Plot.Type.ControlRetinue)
		end)
	end
end

function NonPlayerActions(Menu)
	local Person = Menu.Guy

	GeneralActions(Menu, World.GetPlayer(), Person)
end

function DisplayPlotsAgainst(Menu, Owner)
	for Elem in Owner:PlotsAgainst():Front() do
		Menu:CreateLabel("Plot")
		--[[Menu:CreateButton(Elem:TypeStr(),
			function()
				Menu.Plot = Elem
				DisplayPlotMenu(Menu, Left, Right)
			end)--]]
	end
end

function DisplayFamilyFields(Menu) 
	local Container = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	local Table = Container:CreateTable(5, 16)	
	local Skin = Container:GetSkin()
	local Font = Skin:Table():GetFont()

	Container:CreateLabel("Fields")
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	
	Table:CreateLabel("Name"):SetFocus(false)
	Table:CreateLabel("Yield"):SetFocus(false)
	Table:CreateLabel("Acres"):SetFocus(false)
	Table:CreateLabel("Status"):SetFocus(false)
	Table:CreateLabel("StatusTime"):SetFocus(false)
	for k, Field in ipairs(Menu.Person:GetFamily():GetFields()) do	
	--	Table:CreateLabel(Field:GetCrop().Name)
		Table:CreateLabel("Foo")
		Table:CreateLabel(Field:GetYield())
		Table:CreateLabel(Field:GetAcres())
		Table:CreateLabel(Field:GetStatus())
		Table:CreateLabel(tostring(Field:StatusCompletion()) .. " days")
	end
	Table:Shrink()
	Container:Shrink()
	return Container
end

function DisplayFamilyGoods(Menu) 
	local Container = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	local Table = Container:CreateTable(2, 16)	
	local Skin = Container:GetSkin()
	local Font = Skin:Table():GetFont()

	Container:CreateLabel("Goods")
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())

	Table:CreateLabel("Name"):SetFocus(false)
	Table:CreateLabel("Quantity"):SetFocus(false)
	for val in Menu.Person:GetFamily():GetGoods():Next() do
		Table:CreateLabel(val:GetBase().Name)
		Table:CreateLabel(val:GetQuantity())
	end
	Container:Shrink()
	return Container
end

function DisplayFamilyAnimals(Menu)
	local Container = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	local Table = CreateAnimalTable(Container, 16)	
	local Skin = Container:GetSkin()
	local Font = Skin:Table():GetFont()

	Container:CreateLabel("Animals")
	for An in Menu.Person:GetFamily():GetAnimals():Next() do
		Table:CreateLabel(An:GetBase().Name)
		Table:CreateLabel(An:GetNutrition())
		Table:CreateLabel(An:GetAge())
		Table:CreateLabel(GenderName(An))
	end
	Container:Shrink()
	return Container
end

function DisplayManageHousehold(Menu)
	Menu:Clear()	
	Menu.Agriculture = DisplayFamilyFields(Menu)
	Menu.Goods = DisplayFamilyGoods(Menu)
	Menu.Animals = DisplayFamilyAnimals(Menu)
	--[[Left:CreateButton("Buildings",
	 function()
		if(Right ~= nil) then
			Right:Destroy()
		end
		Right = Left:CreateTable(3, 16)
		Right:SetX(401)
		Right:SetCellWidth(Gui.GetDefaultFont():FontWidth() * 8)
		Right:SetCellHeight(Gui.GetDefaultFont():FontHeight())
		Right:CreateLabel("Width"):SetFocus(false)
		Right:CreateLabel("Length"):SetFocus(false)
	end)--]]
	Menu:CreateButton("Back",
		function(Widget)
		Menu:Clear()
		DisplayViewPerson(Menu)
	end)
end


function DisplayPersonStats(Menu, Guy)
	local Person = Menu.Guy
	local Container = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	local Skin = Container:GetSkin()

	Container:SetSkin(Gui.GetSkin("Header"))
	Container:CreateLabel("Stats")
	Container:SetSkin(Skin)
	BGStatsContainer(Container, Guy)
	Container:Shrink()
	return Container
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

function DisplayFamily(Menu, Family)
	local Table = nil
	local PersonTable = nil
	local Guy = Menu.Guy
	local Label = nil 
	local Table = nil
	--local Container = Gui.VerticalContainer(0, 0, Menu:GetWidth(), Menu:GetHeight(), Menu)
	--local TempSkin = Container:GetSkin()

	--Container:SetSkin(Gui.GetSkin("Header"))
	--Label = Container:CreateLabel("Family")
	--Container:SetSkin(TempSkin);
	PersonTable = Family:GetPeople() 
	Table = CreatePersonTable(Menu, #PersonTable)
	FillPersonTable(Table, PersonTable)
	--Label:SetX(Table:GetWidth() / 2 - Label:GetWidth() / 2)
	--Container:Shrink()
	--return Container
	return Table
end

function DisplayRecruitStats(Menu)
	local Guy = Menu.Guy	
	local Person = Guy:GetPerson()
	local Retinue = Person:Retinue()
	local Warriors = nil 
	local Branches = Retinue:GetBranches()
	if Retinue == nil then
		return	
	end
	Warriors = Retinue:Warriors()
	Menu:Clear()
	Menu:Paragraph("You currently have " .. Warriors:GetSize() .. " warriors in your retinue.")
	Menu:Paragraph("The leader of the retinue is " .. Retinue:Leader():GetPerson():GetName() .. " and has " .. Retinue:Leader():Glory() .. " glory.")
	for Val in ipairs(Branches) do
		Menu:CreateButton(Val:Leader():GetPerson():GetName(),
			function(Widget)
				DisplayRecruitStats(Menu)
			end)
		Menu:CreateLabel("Troops: " .. Val:TroopCt())
	end
	FillWarriorTable(CreateWarriorTable(Right, Warriors:GetSize()), Warriors)
end

function DisplayRelation(Menu, Left, Right)
	local Guy = Menu.Guy
	local Relation = World:GetPlayer():GetRelation(Guy)
	local List = Relation:GetRelationList()

	Right:Clear()
	FillRelationList(CreateRelationTable(Right, #List), List)
end

function EstimateHarvest(Fields)
	local Estimate = 0

	for k, Field in ipairs(Fields) do 
		local Crop = Field:GetCrop()

		Estimate = Estimate + (Field:GetAcres() * Crop.PerAcre * (Crop.YieldMult - 1))
	end
	return Estimate / 365
end

function DisplayGeneral(Menu, Guy) 
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Container = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	local Table = nil 
	local Family = Guy:GetPerson():GetFamily()
	local Acres = 1

	for k, Field in ipairs(Family:GetFields()) do
		Acres = Acres + Field:GetAcres() +  Field:GetUnusedAcres()
	end
	Container:SetSkin(Gui.GetSkin("Header"))
	Container:CreateLabel("General")
	Container:SetSkin(Skin)
	Table = Container:CreateTable(2, 7)
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	Table:CreateLabel("Age")
	Table:CreateLabel(Guy:GetPerson():GetAge())
	Table:CreateLabel("Caste")
	Table:CreateLabel(Family:GetCasteName()) 
	Table:CreateLabel("Popularity")
	Table:CreateLabel(Guy:Popularity()) 
	Table:CreateLabel("Glory")
	Table:CreateLabel(Guy:Glory()) 
	Table:CreateLabel("Stored")
	Table:CreateLabel(string.format("%0.2f", Family:GetNutrition() / (Family:GetNutritionReq() * 365)))
	--if Guy:GetFamily():Job():GetName() == "Farmer" then
		Table:CreateLabel("Harvest")
		Table:CreateLabel(string.format("%0.2f", EstimateHarvest(Family:GetFields()))) 
		Table:CreateLabel("Acres")
		Table:CreateLabel(Acres) 
	--end
	Table:Shrink()
	Container:Shrink()
	return Container
end

function DisplayViewPerson(Menu)
	local Guy = Menu.Guy
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()

	Menu:OnNewChild(Container.Vertical)
	Menu:SetSkin(Gui.GetSkin("Header"))
	Menu.PersonName = Menu:CreateLabel(Menu.Person:GetName())
	Menu:SetSkin(Skin)
	Menu.Info = Gui.HorizontalContainer(Menu, Menu:GetWidth(), Menu:GetHeight()) 
	Menu.Info.Stats = DisplayPersonStats(Menu.Info, Menu.Guy)
	Menu.Info.General = DisplayGeneral(Menu.Info, Menu.Guy)
	Menu.Info:Shrink()

	Menu.Traits = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	Menu.Traits:SetSkin(Gui.GetSkin("Header"))
	Menu.Traits:CreateLabel("Traits")
	Menu.Traits:SetSkin(Skin)
	Menu.Traits.List = Gui.HorizontalContainer(Menu.Traits, Menu.Traits:GetWidth(), Menu.Traits:GetHeight())
	for k in Menu.Guy:TraitList() do 
		Menu.Traits.List:CreateLabel(k:GetName())
	end	
	Menu.Traits.List:Shrink()
	Menu.Traits:Shrink()
	Menu:CreateButton("Actions",
		function(Widget)
			Gui.CreateWindow("ActionMenu", {Owner = World.GetPlayer(), Target = Menu.Guy})
		end)
	Menu.Views = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	Menu.Views.Buttons = Gui.HorizontalContainer(Menu.Views, Menu.Views:GetWidth(), Menu.Views:GetHeight())
	local Foo = Menu.Views.Buttons:CreateButton("Family",
		function(Widget)
			if Menu.Views.Context ~= nil then
				Menu.Views.Context:Destroy()
			end
			Menu.Views.Context = DisplayFamily(Menu.Views, Menu.Person:GetFamily())
		end)
	--[[Menu.Views.Buttons:CreateButton("Household",
		function(Widget)
			DisplayManageHousehold(Menu.Views)
		end)--]]
	Menu.Views.Buttons:CreateButton("Big Guys",
		function(Widget)
			if Menu.Views.Context ~= nil then
				Menu.Views.Context:Destroy()
			end
			Menu.Views.Context = DisplayBigGuys(Menu.Views, Menu.Guy)
		end)
	Menu.Views.Buttons:CreateButton("Plots",
		function(Widget)
			if Menu.Views.Context ~= nil then
				Menu.Views.Context:Destroy()
			end
			if Menu.Guy ~= World.GetPlayer() then
				Menu.Views.Context = DisplayPlots(Menu.Views, Menu.Guy)
			else
				Menu.Views.Context = DisplayPlotsAgainst(Menu.Views, Menu.Guy)
			end
		end)
	Menu.Views.Buttons:Shrink()
	Menu.Views:SetHeight(Menu.Views:GetHeight() - 25)
	Menu:CreateButton("Close",
		function(Widget)
			Widget:GetParent():Close()
		end)

end

function Menu.Init(Menu, Person)
	local Guy = World.GetBigGuy(Person.Person)
	local Family = Person.Person:GetFamily()

	Menu.Person = Person.Person
	Menu.Guy = Guy
	if World.GetPlayer():GetPerson() ~= Person and Guy ~= nil then
		local Rel = Guy:GetRelation(World.GetPlayer())
		if Rel == nil then
			Rel = 0
		else
			Rel = Rel:GetOpinion()
		end
	end
	DisplayViewPerson(Menu)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
