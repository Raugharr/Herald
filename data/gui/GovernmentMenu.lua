Menu.__savestate = false;
Menu.moveable = true;

function Menu.Init(Menu, Data)
--	Menu.VoteCont = GUI.VerticalContainer(0, 0, 512, 100, 0, {0, 0, 0, 0})
	Menu.Government = Data["Settlement"]
--	Menu.Reform = Menu.Government:GetReform()
	
	Menu.TitleCon = GUI.HorizontalContainer(0, 0, Menu:GetWidth(), 30, 0, {0, 0, 0, 0}, Menu)
	Menu.TitleCon:SetFocus(false)
	Menu.Title = Menu.TitleCon:CreateLabel("Government")
	Menu.Title:SetFocus(false)
	Menu.Title:SetX(Menu.TitleCon:GetHorizontalCenter(Menu.Title))
--	Menu.VoteCont:SetFocus(false)
--	if Menu.Reform ~= nil then
--		Menu.VoteLabel = Menu.VoteCont:CreateLabel(Menu.Reform:GetVotes() .. " votes for passing  out of " .. Menu.Reform:GetMaxVotes() .. " total votes.")
--	end
	
	Menu:CreateLabel(Menu.Government:Rule() .. " " .. Menu.Government:Structure() .. " " .. Menu.Government:Type()):SetFocus(false)
	for v in Menu.Government:PossibleReforms():Front() do
		Menu:CreateButton(v:GetName(),
			function()
				Menu.Government:PassReform(v)
			 end)
	end
	Menu:CreateButton("Back",
		function()
			GUI.PopMenu()
		end)
end

function Menu.Think(Menu)

end

function Menu.Quit(Menu)

end
