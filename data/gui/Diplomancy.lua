Menu.Width = 500
Menu.Height = 600
Menu.moveable = true;

function Menu.Init(Menu, Data)
	Menu:OnNewChild(Container.Vertical)

	local PlayerGov = World.GetPlayer():GetSettlement():GetGovernment()

	Menu.Government = Data.Government
	Menu.Action = nil
	Menu:CreateButton("Ally",
		function(Widget)
			--Player:CreateTreaty(Menu.Government, Treaty.Alliacne, 12)
			Menu.Score = PlayerGov:ScoreAlliance(Menu.Government)
			Menu.ScoreLabel:SetText(Menu.Score)
		end)
	Menu:CreateButton("Non-agression",
		function(Widget)
			Player:CreateTreaty(Menu.Government, Treaty.CeaseFire, 12)
		end)
	Menu:CreateButton("Swear Fealty",
		function(Widget)
			PlayerGov:SwearFealty(Menu.Government)
			--Menu.Score = PlayerGov:ScoreFealty(Menu.Government)
			--Menu.ScoreLabel:SetText(Menu.Score)
		end)
	Menu:CreateButton("Demand Fealty",
		function(Widget)
		end)
	Menu.ScoreLabel = Menu:CreateLabel("0")	
	Menu:CreateButton("Accept",
		function(Widget)
			if Menu.Action == nil then return end
			Menu.Action()
		end)
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
end
