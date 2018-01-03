Menu.moveable = true
Menu.Width = 600
Menu.Height = 400

function Menu.Init(Menu, Data)
	local Goal = Data.Goal
	local FactId = Data.Faction
	local Table = Menu:CreateTable(3, 2)
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local Gov = FactId:GetSettlement():GetGovernment()

	Menu:OnNewChild(Container.Vertical)
	Table:SetCellWidth(Font:Width() * 16)
	Table:SetCellHeight(Font:Height() * 7)
	for k, Pol in pairs(World.Policies()) do
		local CurrPolicy = Gov:GetPolicyCategory(Pol)
		local Cont = Table:CreateContainer(Table:GetCellWidth(), Table:GetCellHeight(), Container.Vertical)
		local ContSkin = Cont:GetSkin()

		Cont:SetSkin(Gui.GetSkin("Header"))
		Cont:CreateLabel(Pol:GetName())
		Cont:SetSkin(ContSkin)
		for j, Opt in ipairs(Pol:Options()) do
			if j == CurrPolicy - 1 or j == CurrPolicy + 1 then
				Cont:CreateButton(Opt:GetName(),
				function(Widget)
					FactId:SetGoal(Goal, Pol, j)
					Gui.CreateWindow("FactionBet", {Faction = FactId})
					Menu:Close()
				end)
			else
				Cont:CreateLabel(Opt:GetName())
			end
		end
	end
end

function Menu.Quit(Menu)

end


