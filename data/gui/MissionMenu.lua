function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	Menu.Mission = Data["Mission"]
	
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = TitleCon:CreateLabel(Mission.Name)
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(TitleCon:GetHorizontalCenter(Title))
	Menu.Screen:Paragraph(GUI.GetFont("Elementary_Gothic_Bookhand.ttf", 12), Mission.Description)
	for k, v in pairs(Menu.Mission.OptionNames) do
		Menu.Screen:CreateLabel(v):OnKey("Enter", "Released", 
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