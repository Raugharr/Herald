ViewPersonMenu = { }

function ViewPersonMenu.Init(Width, Height, Person)
	local Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Menu = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0}, Screen)
	Display = nil
	local String = Person.Name .. " is of the family " .. Person.Family:GetName() .. "."
	
	Menu:CreateTextBox(String):SetFocus(false)
	Menu:CreateTextBox("Skills")
	Menu:CreateTextBox("Agriculture"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(3, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(Display:GetFont():FontWidth() * 8)
			Display:SetCellHeight(Display:GetFont():FontHeight())
			
			Display:CreateTextBox("Name"):SetFocus(false)
			Display:CreateTextBox("Yield"):SetFocus(false)
			Display:CreateTextBox("Acres"):SetFocus(false)
			for Field in Person.Family:GetFields():Next() do	
				Display:CreateTextBox(Field:GetCrop().Name)
				Display:CreateTextBox(Field:GetYield())
				Display:CreateTextBox(Field:GetAcres())
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
			for Good in Person.Family:GetGoods():Next() do
				Display:CreateTextBox(Good:GetBase().Name)
				Display:CreateTextBox(Good:GetQuantity())
			end
		end)
	Menu:CreateTextBox("Animals"):OnKey("Enter", "Released",
		function()
			if(Display ~= nil) then
				Display:Destroy()
			end
			Display = Screen:CreateTable(2, 16, 0, {0, 0, 0, 0})
			Display:SetCellWidth(Display:GetFont():FontWidth() * 8)
			Display:SetCellHeight(Display:GetFont():FontHeight())
			Display:CreateTextBox("Name"):SetFocus(false)
			Display:CreateTextBox("Nutrition"):SetFocus(false)
			for An in Person.Family:GetAnimals():Next() do
				Display:CreateTextBox(An:GetBase().Name)
				Display:CreateTextBox(An:GetNutrition())
			end
		end)
	Menu:CreateTextBox("Back"):OnKey("Enter", "Released",
		function()
			GUI.Close()
		end)
	
	return false
end
