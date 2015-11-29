Menu.__savestate = false;
Menu.moveable = false;

function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	Menu.Mission = Data["Mission"]
	Menu.Data = Data["Data"]
	
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel(Menu.Mission:GetName())
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.Screen:Paragraph(Menu.Mission:GetDescription())
	for k, v in ipairs(Menu.Mission:GetOptions()) do
		if v:ConditionSatisfied() == true then
			Menu.Screen:CreateButton(v:GetName(),
			function()
				Menu.Mission:ChooseOption(Menu.Data, k - 1)
				Menu.Screen:Close()
			end)
		end
	end
	return Menu.Screen
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end