ViewPersonMenu = { }

function ViewPersonMenu.Init(Width, Height, Person)
	local Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Menu = GUI.VerticalContainer(0, 0, 400, Height, 0, {0, 0, 0, 0}, Screen)
	Display = nil
	local String = Person:GetName() .. " is of the family " .. Person:GetFamily():GetName() .. ". " .. "He owns " .. Person:GetFamily():GetBuildingCt() .. " buildings and " .. Person:GetFamily():GetAnimalCt() .. " animals."
	
	Menu:Paragraph(GUI.GetFont("Plain Germanica.ttf", 21), String)
	Menu:CreateLabel("Skills")
	Menu:CreateLabel("Agriculture"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(5, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Display:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			
			Display:CreateLabel("Name"):SetFocus(false)
			Display:CreateLabel("Yield"):SetFocus(false)
			Display:CreateLabel("Acres"):SetFocus(false)
			Display:CreateLabel("Status"):SetFocus(false)
			Display:CreateLabel("StatusTime"):SetFocus(false)
			for Field in Person:GetFamily():GetFields():Next() do	
				Display:CreateLabel(Field:GetCrop().Name)
				Display:CreateLabel(Field:GetYield())
				Display:CreateLabel(Field:GetAcres())
				Display:CreateLabel(Field:GetStatus())
				Display:CreateLabel(Field:GetStatusTime())
			end
		end)
	Menu:CreateLabel("Goods"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(2, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(GetDefaultFont():FontWidth() * 8)
			Display:SetCellHeight(GetDefaultFont():FontHeight())
			Display:CreateLabel("Name"):SetFocus(false)
			Display:CreateLabel("Quantity"):SetFocus(false)
			for Good in Person:GetFamily():GetGoods():Next() do
				Display:CreateLabel(Good:GetBase().Name)
				Display:CreateLabel(Good:GetQuantity())
			end
		end)
	Menu:CreateLabel("Animals"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(3, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
			Display:SetCellHeight(GUI.GetDefaultFont():FontHeight())
			Display:CreateLabel("Name"):SetFocus(false)
			Display:CreateLabel("Nutrition"):SetFocus(false)
			Display:CreateLabel("Age"):SetFocus(false)
			for An in Person:GetFamily():GetAnimals():Next() do
				Display:CreateLabel(An:GetBase().Name)
				Display:CreateLabel(An:GetNutrition())
				Display:CreateLabel(PrintYears(An:GetAge()))
			end
		end)
	Menu:CreateLabel("Buildings"):OnKey("Enter", "Released",
	 function()
		if(Display ~= nil) then
			Display:Destroy()
		end
		Display = Screen:CreateTable(3, 16, 0, {0, 0, 0, 0})
		Display:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
		Display:SetCellHeight(GUI.GetDefaultFont():FontHeight())
		Display:CreateLabel("Width"):SetFocus(false)
		Display:CreateLabel("Length"):SetFocus(false)
	 end)
	Menu:CreateLabel("Back"):OnKey("Enter", "Released",
		function()
			GUI.PopMenu()
		end)
	
	return false
end