Menu.moveable = true
Menu.Width = 400
Menu.Height = 300

function Menu.Init(Menu, Data)
	Menu.TextBox = Data["Text"]
	
	if Menu.TextBox == nil then
		Menu.TextBox = ""
	end
	Menu.Text = Menu:Paragraph(Menu.TextBox)
	Menu.TextBox = Menu:CreateButton("Ok", 
		function()
			Menu:Close()
		end)
	Menu.TextBox:Below(Menu.Text)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
