Menu.__savestate = false;
Menu.moveable = false;

function Menu.Init(Menu, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Menu:GetWidth(), Menu:GetHeight())
	Menu.Mission = Data["Mission"]
	Menu.Data = Data["Data"]
	
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Menu:GetWidth(), 30, Menu.Screen)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel(Menu.Mission:GetName())
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.Screen:Paragraph(Data["Description"])
	for k, v in ipairs(Menu.Mission:GetOptions()) do
		if v:ConditionSatisfied() == true then
			Menu.Screen:CreateButton(v:GetName(),
			function()
				Menu.Mission:ChooseOption(Menu.Data, k - 1)
				Menu.Screen:Close()
			end)
		end
	end
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
