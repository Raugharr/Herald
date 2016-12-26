Menu.moveable = true
Menu.Width = 500
Menu.Height = 600

local function ShowGeneral(Menu, Faction)
	local Table = Menu:CreateTable(2, 6)	
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()

	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	Table:CreateLabel("Leader")
	Table:CreateLabel(Faction:GetLeader():GetName())
	Table:CreateLabel("Power")
	Table:CreateLabel(Faction:GetPower())
	Table:CreateLabel("Power Gain")
	Table:CreateLabel(Faction:GetPowerGain())
	Table:CreateLabel("Members")
	Table:CreateLabel(Faction:GetMembers())
end

local function ShowCastePower(Menu, Faction)
	local Table = Menu:CreateTable(3, World.CasteNum + 1)
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Idx = 1
	local Power = Faction:GetCastePower()
	local Weight= Faction:GetCasteWeight()

	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	Table:CreateLabel("Caste")
	Table:CreateLabel("Power")
	Table:CreateLabel("Weight")
	for i, Caste in ipairs(World.Castes) do
		Table:CreateLabel(Caste)
		Table:CreateLabel(Power[Idx])
		Table:CreateLabel(Weight[Idx] .. "%")
		Idx = Idx + 1
	end
end

local function DisplayGoals(Menu, Faction)
	local Container = Gui.VerticalContainer(Menu, Menu:GetWidth(), 200)
	Container:CreateButton("Select goal",
		function(Widget)
			if Faction:CanPassGoal() == false then
				return
			end
			if Widget.Open == nil or Widget.Open == false then
			for k, v in pairs(Faction:ListGoals()) do
					Container:CreateButton(v,
						function(Widget)
							Faction:SetGoal(k)
							Gui.CreateWindow("FactionBet", {Faction = Faction})
						end)	

					end
				Widget.Open = true
			end
		end)
end

function Menu.Init(Menu, Data)
	local Player = Data.Player
	local Settlement = Player:GetSettlement()
	local Faction = Player:GetFaction()
	local Skin = Menu:GetSkin()

	Menu:OnNewChild(Container.Vertical)
	Menu:SetSkin(Gui.GetSkin("Header"))
	Menu:CreateLabel(Faction:GetName() .. " Faction")
	Menu:SetSkin(Skin)
	ShowGeneral(Menu, Faction)
	ShowCastePower(Menu, Faction)
	DisplayGoals(Menu, Faction)
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
end

function Menu.Think(Menu)

end

