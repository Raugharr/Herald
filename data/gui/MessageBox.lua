Menu.__savestate = false

function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	Menu.TextBox = Data["Text"]
	
	if Menu.TextBox == nil then
		Menu.TextBox = ""
	end
	Menu.Text = GUI.Paragraph(Menu.TextBox)
	Menu.TextBox = GUI.CreateButton("Ok", 
		function()
			GUI.PopMenu()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end