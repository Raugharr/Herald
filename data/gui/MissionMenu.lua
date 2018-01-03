Menu.moveable = false;
Menu.Width = 512
Menu.Height = 512

function Menu.Init(Menu, Data)
	Menu.Mission = Data["Mission"]
	Menu.Data = Data["Data"]
	local Skin = nil	

	Menu.TitleCon = Gui.HorizontalContainer(Menu, Menu:GetWidth(), 30)
	Skin = Menu.TitleCon:GetSkin()
	Menu.TitleCon:SetSkin(Gui.GetSkin("Header"))
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel(Menu.Mission:GetName())
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	Menu:SetSkin(Skin)
	Menu.Description = Menu:Paragraph(Data["Description"])
	Menu.Description:Below(Menu.TitleCon)
	Menu.ButtonCont = Gui.VerticalContainer(Menu, Menu:GetWidth(), Menu:GetHeight())
	Menu.ButtonCont:Below(Menu.Description)
	for k, v in ipairs(Menu.Mission:GetOptions()) do
		if v:ConditionSatisfied(Menu.Data) == true then
			Menu.ButtonCont:CreateButton(v:GetName(Menu.Data),
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
