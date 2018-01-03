Menu.moveable = true
Menu.Width = 300
Menu.Height = 300

function Menu.Init(Menu, Data)
	local Settlement = Data.Settlement

	Menu:OnNewChild(Container.Vertical)
	Menu:Paragraph("Select an action to take on the " .. string.lower(Settlement:GetType()) .. " " .. Settlement:GetName())
	Menu:CreateButton("Take slaves",
		function(Widget)
			World.SetOnClick(World.Action.RaiseArmy, Settlement:Self(), Army.Goal.Slaves)
			Menu:Close()
		end)
	Menu:CreateButton("Pillage",
		function(Widget)
			World.SetOnClick(World.Action.RaiseArmy, Settlement:Self(), Army.Goal.Pillage)
			Menu:Close()
		end)
	Menu:CreateButton("Slaughter",
		function(Widget)
			World.SetOnClick(World.Action.RaiseArmy, Settlement:Self(), Army.Goal.Slaughter)
			Menu:Close()
		end)
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end

