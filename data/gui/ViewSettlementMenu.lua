ViewSettlementMenu = { }

function ViewSettlementMenu.Init(Width, Height, Data)
	local Screen = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0})
	local TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Screen)
	local Title = TitleCon:CreateLabel("Herald")
	
	TitleCon:SetFocus(false)
	Title:SetFocus(false)
	Title:SetX(TitleCon:GetHorizontalCenter(Title))
	
	Screen:CreateLabel("Raise Fyrd"):OnKey("Enter", "Released",
		function()
			Data["Settlement"]:RaiseArmy()
		end)
	Screen:CreateLabel("Back"):OnKey("Enter", "Released",
		function()
			GUI.PopMenu()
		end)
	return false
end

function ViewSettlementMenu.Think(Menu)

end

function ViewSettlementMenu.Quit(Menu)

end