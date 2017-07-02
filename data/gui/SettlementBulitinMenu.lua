Menu.moveable = true
Menu.Width = 500
Menu.Height = 600

function Menu.Init(Menu, Data)
	local Table = Menu:CreateTable(3, 16)
	local Skin = Table:GetSkin()
	local Font = Skin:Table():GetFont()

	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	Table:CreateLabel("Name");
	Table:CreateLabel("Owner");
	Table:CreateLabel("Days Left");
	for Item in Data["Settlement"]:GetBulletins():Next() do
		if Item:GetOwner() ~= World.GetPlayer() then
			Table:CreateButton(Item:GetName(),
				function()
					CallMissionById(Item:GetMission(), Item:GetOwner(), World.GetPlayer())
				end)
		else
			Table:CreateLabel(Item:GetName())
		end
		Table:CreateLabel(Item:GetOwner():GetName())
		Table:CreateLabel(Item:DaysLeft()) 
	end	

	Menu:CreateButton("Close",
		function()
			Menu:Close()
		end):Below(Table)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
