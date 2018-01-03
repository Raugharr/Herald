Menu.moveable = false;
Menu.Width = 400
Menu.Height = 600

local function ListCommanders(Army)
	ComandList = {}
	
	for Warbands in ipairs(Army:GetWarbands()) do
		table.insert(ComandList, Warbands:GetLeader():GetPerson():GetName())
	end
	return ComandList
end

function Menu.Init(Menu, Data)
	local Army = Data.Army
	local Leader = Army:GetLeader()
	
	Menu:CreateLabel("Leader: " .. Leader:GetName())
	Menu:CreateButton("Commanders",
		function(Widget)
			local Commanders = ListCommanders(Army)
			
			Table = Menu:CreateTable(4, 10)
			Table:SetCellWidth(Font:Width() * 8)
			Table:SetCellHeight(Font:Height())
			
			for Elem in ipairs(Commanders) do
				Menu:CreateLabel(Elem)
			end
		end)
	Menu:CreateLabel("Troops: " .. Army:GetSize())
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end