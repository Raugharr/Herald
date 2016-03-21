Menu.__savestate = false;
Menu.moveable = true;


function Menu.Init(Menu, Data)
	local Table = Menu:CreateTable(4, 16)

	Table:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(GUI.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name");
	Table:CreateLabel("Description");
	Table:CreateLabel("Owner");
	Table:CreateLabel("Days Left");
	for Item in Data["Settlement"]:GetBulitins():NextItr() do
		Table:CreateButton(Item:GetName(),
			function()
				CallMissionById(Item:GetMission(), Item:GetOwner(), World.GetPlayer())
			end)
		Table:CreateLabel("a")
		Table:CreateLabel(Item:GetOwner():GetName())
		Table:CreateLabel(Item:DaysLeft()) 
	end	

	Menu:CreateButton("Close",
		function()
			Menu.Screen:Close()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
