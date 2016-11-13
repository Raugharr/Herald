Menu.__savestate = false
Menu.moveable = true

function Menu.Init(Menu, Data)
	local Loser = Data["Loser"]
	Menu.Screen = Gui.VerticalContainer(0, 0, 400, Menu:GetHeight(), Menu)
	if Null(Loser) == false then
		Menu.Screen:Paragraph(Data["Winner"]:GetName() .. " has defeated " .. Loser:GetName() .. ".")
		Menu.Screen:CreateButton("Kill " .. Loser:GetName(),
			function()
				Looser:Kill()
				Menu:Close()
			end)
		Menu.Screen:CreateButton("Banish " .. Loser:GetName(),
			function()
				Menu:Close()
			end)
		Menu.Screen:CreateButton("Forgive " .. Loser:GetName(),
			function()
				Menu:Close()
			end)
	else
		Menu.Screen:Paragraph(Data["Winner"]:GetName() .. " has won.")
		Menu.Screen:CreateButton("Ok", 
			function()
				Menu:Close()
			end)
	end
	
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
