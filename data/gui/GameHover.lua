Menu.moveable = false 

function Menu.Init(Menu, Data)
	local Settlement = Data.Settlement

	Menu:CreateLabel("Pop: " .. Settlement:GetPopulation())
	
end
