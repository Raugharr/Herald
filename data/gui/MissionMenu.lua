function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	Menu.Mission = Data["Mission"]
	
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel(Menu.Mission.Name)
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.Screen:Paragraph(GUI.GetFont("Elementary_Gothic_Bookhand.ttf", 12), Menu.Mission.Description)
	for k, v in pairs(Menu.Mission.OptionNames) do
		Menu.Screen:CreateLabel(v):OnClick(
		function()
			GUI.SendMessage("Mission", Menu.Mission.Options(Data["BigGuy"], World.GetDate())[k])
			GUI.PopMenu()
		end)
	end
	return false
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end