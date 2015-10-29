Menu.__savestate = false
Menu.moveable = true

function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	Menu.TextBox = Data["Text"]
	
	if Menu.TextBox == nil then
		Menu.TextBox = ""
	end
	Menu.Text = Menu.Screen:Paragraph(Menu.TextBox)
	Menu.TextBox = Menu.Screen:CreateButton("Ok", 
		function()
			Menu.Screen:Close()
		end)
	return Menu.Screen
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end