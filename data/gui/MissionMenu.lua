Menu.__savestate = false;
Menu.moveable = false;

function Menu.Init(Menu, Data)
	Menu.Mission = Data["Mission"]
	Menu.Data = Data["Data"]
	
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Menu:GetWidth(), 30, Menu)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel(Menu.Mission:GetName())
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu.Description = Menu:Paragraph(Data["Description"])
	Menu.Description:Below(Menu.TitleCon)
	Menu.ButtonCont = GUI.VerticalContainer(0, 0, Menu:GetWidth(), Menu:GetHeight(), Menu)
	Menu.ButtonCont:Below(Menu.Description)
	for k, v in ipairs(Menu.Mission:GetOptions()) do
		if v:ConditionSatisfied() == true then
			Menu.ButtonCont:CreateButton(v:GetName(),
			function()
				Menu.Mission:ChooseOption(Menu.Data, k - 1)
				Menu:Close()
			end)
		end
	end
	Menu.ButtonCont:Shrink()
	--Menu.Description:Below(Menu.ButtonCont)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
