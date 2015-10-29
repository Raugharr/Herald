Menu.__savestate = false;
Menu.moveable = true;

function Menu.Init(Menu, Width, Height, Data)
	Menu.Screen = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0})
	Menu.VoteCont = GUI.VerticalContainer(0, 0, 512, 100, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.Government = Data["Settlement"]
	Menu.Reform = Menu.Government:GetReform()
	
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Menu.Screen)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel("Government")
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
	--Screen:Paragraph(Mission.Description)
	--Government:PossibleReforms():Front():Data():GetName()
	Menu.VoteCont:SetFocus(false)
	if Menu.Reform ~= nil then
		Menu.VoteLabel = Menu.VoteCont:CreateLabel(Menu.Reform:GetVotes() .. " votes for passing  out of " .. Menu.Reform:GetMaxVotes() .. " total votes.")
	end
	
	Menu.Screen:CreateLabel(Menu.Government:Rule() .. " " .. Menu.Government:Structure() .. " " .. Menu.Government:Type()):SetFocus(false)
	for v in Menu.Government:PossibleReforms():Front() do
		Menu.Screen:CreateButton(v:GetName(),
			function()
				Menu.Government:PassReform(v)
			 end)
	end
	Menu.Screen:CreateButton("Back",
		function()
			GUI.PopMenu()
		end)
	return Menu.Screen
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end