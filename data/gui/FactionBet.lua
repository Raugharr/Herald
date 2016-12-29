Menu.Width = 400
Menu.Height = 300
Menu.moveable = true;

function Menu.Init(Menu, Data)
	local Bet = 0
	local BetConst = 10
	local Faction = Data.Faction;

	Menu:OnNewChild(Container.Vertical)
	local BetMenu = Menu:CreateContainer(Menu:GetWidth(), Menu:GetHeight(), Container.Horizontal)
	local BetLbl = nil
	BetMenu:CreateButton("Minus",
		function(Widget)
			if Bet - BetConst >= 0 then
				Bet = Bet - BetConst
				BetLbl:SetText(tostring(Bet))
			end
		end)
	BetLbl = BetMenu:CreateLabel("0")	
	BetMenu:CreateButton("Add",
		function(Widget)
			if Bet + BetConst < Faction:GetPower() then
				Bet = Bet + BetConst
				BetLbl:SetText(tostring(Bet))
			end
		end)
	BetMenu:Shrink()
	Menu:CreateButton("Bet",
		function(Widget)
			Faction:SetBet(Bet)
			Menu:Close()
		end)
end

function Menu.Think(Menu)

end
