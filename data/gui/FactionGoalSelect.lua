Menu.moveable = true
Menu.Width = 300
Menu.Height = 400

function Menu.Init(Menu, Data)
	local Goal = Data.Goal
	local FactId = Data.Faction
	local Table = Menu:CreateTable(1, World.CasteNum)
	local Skin = Menu:GetSkin()
	local Font = Skin:Table():GetFont()
	local ToCaste = 0
	local FromCaste = 0

	Menu:OnNewChild(Container.Vertical)
	Table:SetCellWidth(Font:Width() * 8)
	Table:SetCellHeight(Font:Height())
	local Text = Menu:CreateLabel("Caste to add power")
		for i, Caste in ipairs(World.Castes) do
			local Button = Table:CreateButton(Caste,
				function(Widget)
					ToCaste = Widget.Caste;
					Text:SetText("Cate to take power")
					for i, Child in ipairs(Table:Children()) do
						Child:OnClick(
							function(Widget)
								FactId:SetGoal(Goal, ToCaste, Widget.Caste)
								Gui.CreateWindow("FactionBet", {Faction = FactId})
								Menu:Close()
							end)
					end
				end)
			Button.Caste = i - 1;
		end
end

function Menu.Quit(Menu)

end

