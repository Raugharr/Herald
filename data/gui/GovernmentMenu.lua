GovernmentMenu = { }

function GovernmentMenu.Init(Width, Height, Data)
	local Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Government = Data
	
	local TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Screen)
	TitleCon:SetFocus(false)
	local Title = TitleCon:CreateLabel("Government")
	Title:SetFocus(false)
	Title:SetX(TitleCon:GetHorizontalCenter(Title))
	--Screen:Paragraph(GUI.GetFont("Elementary_Gothic_Bookhand.ttf", 12), Mission.Description)
	--Government:PossibleReforms():Front():Data():GetName()
	Screen:CreateLabel(Government:Rule() .. " " .. Government:Structure() .. " " .. Government:Type()):SetFocus(false)
	for v in Government:PossibleReforms():Front() do
		Screen:CreateLabel(v:GetName()):OnKey("Enter", "Released",
			function()
				Government:PassReform(v)
			 end)
	end
	return false
end