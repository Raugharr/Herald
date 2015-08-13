function NonPlayerActions(Menu, Person)
	Menu:CreateButton("Influence", 
		function()
			World.GetPlayer():SetAction(1, Person)
		end)
end

function Menu.Init(Menu, Width, Height, Person)
	Menu.Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	Menu.MenuBar = GUI.VerticalContainer(0, 0, 400, Height, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.Display = nil
	local String = Person:GetName() .. " is of the family " .. Person:GetFamily():GetName() .. ". " .. "He owns " .. Person:GetFamily():GetBuildingCt() .. " buildings and " .. Person:GetFamily():GetAnimalCt() .. " animals."
	local Guy = nil
	
	Guy = World.GetBigGuy(Person)
	if World.GetPlayer():GetPerson() ~= Person and Guy ~= nil then
		local Rel = Guy:GetRelation(World.GetPlayer())
		if Rel == nil then
			Rel = 0
		else
			Rel = Rel:GetOpinion()
		end
		String = String .. " Their opinion of us is " .. Rel
	end
	
	Menu.MenuBar:Paragraph(GUI.GetFont("Plain Germanica.ttf", 21), String)
	Menu.MenuBar:CreateLabel("Skills")
	Menu.MenuBar:CreateButton("Agriculture",
		function()
			if(Menu.Display ~= nil) then
				Menu.Display:Destroy()
			end
			Menu.Display = Menu.Screen:CreateTable(5, 16, 0, {0, 0, 0, 0})
			Menu.Display:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Menu.Display:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			
			Menu.Display:CreateLabel("Name"):SetFocus(false)
			Menu.Display:CreateLabel("Yield"):SetFocus(false)
			Menu.Display:CreateLabel("Acres"):SetFocus(false)
			Menu.Display:CreateLabel("Status"):SetFocus(false)
			Menu.Display:CreateLabel("StatusTime"):SetFocus(false)
			for Field in Person:GetFamily():GetFields():Next() do	
				Menu.Display:CreateLabel(Field:GetCrop().Name)
				Menu.Display:CreateLabel(Field:GetYield())
				Menu.Display:CreateLabel(Field:GetAcres())
				Menu.Display:CreateLabel(Field:GetStatus())
				Menu.Display:CreateLabel(Field:GetStatusTime())
			end
		end)
	Menu.MenuBar:CreateButton("Goods",
		function()
			if(Menu.Display ~= nil) then
				Menu.Display:Destroy()
			end
			Menu.Display = Menu.Screen:CreateTable(2, 16, 0, {0, 0, 0, 0})
			Menu.Display:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Menu.Display:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			Menu.Display:CreateLabel("Name"):SetFocus(false)
			Menu.Display:CreateLabel("Quantity"):SetFocus(false)
			for val in Person:GetFamily():GetGoods():Next() do
				Menu.Display:CreateLabel(val:GetBase().Name)
				Menu.Display:CreateLabel(val:GetQuantity())
			end
		end)
	Menu.MenuBar:CreateButton("Animals",
		function()
			if(Menu.Display ~= nil) then
				Menu.Display:Destroy()
			end
			Menu.Display = Menu.Screen:CreateTable(3, 16, 0, {0, 0, 0, 0})
			Menu.Display:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Menu.Display:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			Menu.Display:CreateLabel("Name"):SetFocus(false)
			Menu.Display:CreateLabel("Nutrition"):SetFocus(false)
			Menu.Display:CreateLabel("Age"):SetFocus(false)
			for An in Person:GetFamily():GetAnimals():Next() do
				Menu.Display:CreateLabel(An:GetBase().Name)
				Menu.Display:CreateLabel(An:GetNutrition())
				Menu.Display:CreateLabel(PrintYears(An:GetAge()))
			end
		end)
	Menu.MenuBar:CreateButton("Buildings",
	 function()
		if(Menu.Display ~= nil) then
			Menu.Display:Destroy()
		end
		Menu.Display = Menu.Screen:CreateTable(3, 16, 0, {0, 0, 0, 0})
		Menu.Display:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
		Menu.Display:SetCellHeight(GUI.GetDefaultFont():FontHeight())
		Menu.Display:CreateLabel("Width"):SetFocus(false)
		Menu.Display:CreateLabel("Length"):SetFocus(false)
	end)
	if World.GetPlayer() ~= Guy then
		NonPlayerActions(Menu.MenuBar, Guy)
	end
	Menu.MenuBar:CreateButton("Back",
		function()
			Menu.Screen:Close()
		end)
	return false
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end