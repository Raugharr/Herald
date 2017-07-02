Menu.moveable = true
Menu.Width = 300
Menu.Height = 200

function Menu.Init(Menu, Data)
	local Settlement = Data.Settlement

	Menu:OnNewChild(Container.Vertical)
	Menu:CreateButton("Take slaves",
		function(Widget)
			World.SetOnClick(World.Action.RaiseArmy, Settlement.__self, Army.Goal.Slaves)
			Menu:Close()
		end)
	Menu:CreateButton("Pillage",
		function(Widget)
			World.SetOnClick(World.Action.RaiseArmy, Settlement.__self, Army.Goal.Pillage)
			Menu:Close()
		end)
	Menu:CreateButton("Slaughter",
		function(Widget)
			World.SetOnClick(World.Action.RaiseArmy, Settlement.__self, Army.Goal.Slaughter)
			Menu:Close()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end

