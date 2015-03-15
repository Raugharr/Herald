TakeActionMenu = { }

function TakeActionMenu.Init(Width, Height, Data)
	local Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	
	local TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Screen)
	TitleCon:SetFocus(false)
	local Title = TitleCon:CreateLabel("Take Action")
	Title:SetFocus(false)
	Title:SetX(TitleCon:GetHorizontalCenter(Title))
	Screen:CreateLabel("Construct Building"):OnKey("Enter", "Released",
		function()
			--CreateBuilding({{"One Room House", 10, 15}}, BuildMat("Dirt"), BuildMat("Wood"), BuildMat("Hay"))
		end)
	Screen:CreateLabel("Create Object"):OnKey("Enter", "Released",
		function()
		end)
	Screen:CreateLabel("Exit"):OnKey("Enter", "Released",
		function()
			GUI.PopMenu()
		end)
	return false
end