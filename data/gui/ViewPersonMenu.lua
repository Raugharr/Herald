ViewPersonMenu = { }

function ViewPersonMenu.Init(Width, Height, Person)
	local Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Menu = GUI.VerticalContainer(0, 0, 400, Height, 0, {0, 0, 0, 0}, Screen)
	Display = nil
	local String = Person:GetName() .. " is of the family " .. Person:GetFamily():GetName() .. ". " .. "He owns " .. Person:GetFamily():GetBuildingCt() .. " buildings and " .. Person:GetFamily():GetAnimalCt() .. " animals."
	
	Menu:Paragraph(GUI.GetFont("Plain Germanica.ttf", 21), String)
	Menu:CreateTextBox("Skills")
	Menu:CreateTextBox("Agriculture"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(5, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(Display:GetFont():FontWidth() * 8)
			Display:SetCellHeight(Display:GetFont():FontHeight())
			
			Display:CreateTextBox("Name"):SetFocus(false)
			Display:CreateTextBox("Yield"):SetFocus(false)
			Display:CreateTextBox("Acres"):SetFocus(false)
			Display:CreateTextBox("Status"):SetFocus(false)
			Display:CreateTextBox("StatusTime"):SetFocus(false)
			for Field in Person:GetFamily():GetFields():Next() do	
				Display:CreateTextBox(Field:GetCrop().Name)
				Display:CreateTextBox(Field:GetYield())
				Display:CreateTextBox(Field:GetAcres())
				Display:CreateTextBox(Field:GetStatus())
				Display:CreateTextBox(Field:GetStatusTime())
			end
		end)
	Menu:CreateTextBox("Goods"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(2, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(Display:GetFont():FontWidth() * 8)
			Display:SetCellHeight(Display:GetFont():FontHeight())
			Display:CreateTextBox("Name"):SetFocus(false)
			Display:CreateTextBox("Quantity"):SetFocus(false)
			for Good in Person:GetFamily():GetGoods():Next() do
				Display:CreateTextBox(Good:GetBase().Name)
				Display:CreateTextBox(Good:GetQuantity())
			end
		end)
	Menu:CreateTextBox("Animals"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(3, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(Display:GetFont():FontWidth() * 8)
			Display:SetCellHeight(Display:GetFont():FontHeight())
			Display:CreateTextBox("Name"):SetFocus(false)
			Display:CreateTextBox("Nutrition"):SetFocus(false)
			Display:CreateTextBox("Age"):SetFocus(false)
			for An in Person:GetFamily():GetAnimals():Next() do
				Display:CreateTextBox(An:GetBase().Name)
				Display:CreateTextBox(An:GetNutrition())
				Display:CreateTextBox(PrintYears(An:GetAge()))
			end
		end)
	Menu:CreateTextBox("Buildings"):OnKey("Enter", "Released",
	 function()
		if(Display ~= nil) then
			Display:Destroy()
		end
		Display = Screen:CreateTable(3, 16, 0, {0, 0, 0, 0})
		Display:SetCellWidth(Display:GetFont():FontWidth() * 8)
		Display:SetCellHeight(Display:GetFont():FontHeight())
		Display:CreateTextBox("Width"):SetFocus(false)
		Display:CreateTextBox("Length"):SetFocus(false)
	 end)
	Menu:CreateTextBox("Back"):OnKey("Enter", "Released",
		function()
			GUI.PopMenu()
		end)
	
	return false
end
