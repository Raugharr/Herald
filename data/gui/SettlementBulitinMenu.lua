Menu.__savestate = false;
Menu.moveable = true;


function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.HorizontalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Table = Menu.Screen:CreateTable(4, 16, 0, {0, 0, 0, 0})
	--local Item = Data["Settlement"]:GetBulitins()

	Table:SetCellWidth(GUI.GetDefaultFont():FontWidth() * 8)
	Table:SetCellHeight(GUI.GetDefaultFont():FontHeight())
	Table:CreateLabel("Name");
	Table:CreateLabel("Description");
	Table:CreateLabel("Owner");
	Table:CreateLabel("Days Left");
	for Item in Data["Settlement"]:GetBulitins():NextItr() do
	--while not Null(Item) do
		Table:CreateButton(Item:GetName(),
			function()
				CallMissionById(Item:GetMission(), Item:GetOwner(), World.GetPlayer())
			end)
		Table:CreateLabel("a")
	--	Table:CreateLabel(Item:GetDescription())
		Table:CreateLabel(Item:GetOwner():GetName())
		Table:CreateLabel(Item:DaysLeft()) 
		--Item = Item:Next()
	end	

	Menu.Screen:CreateButton("Close",
		function()
			Menu.Screen:Close()
		end)
	return Menu.Screen
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
