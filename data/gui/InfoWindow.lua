Menu.moveable = false 
Menu.Width = 250
Menu.Height = 300

function CreateInfoWindow(Text) 
	local Window = Gui.CreateWindow("InfoWindow", {Text = Text}) 
	local x = 0
	local y = 0

	x, y = MousePos()
	Window:SetPos(x + 15, y)
	return Window
end

function Menu.Init(Menu, Data)
	Menu.Text = Menu:Paragraph(Data.Text)
end

function Menu.Think(Menu)

end

