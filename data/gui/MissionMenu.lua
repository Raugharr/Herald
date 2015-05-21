MissionMenu = { }

function MissionMenu.Init(Width, Height, Data)
	local Screen = GUI.VerticalContainer(0, 0, Width, Height, 0, {0, 0, 0, 0})
	local Mission = Data["Mission"]
	
	local TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Screen)
	TitleCon:SetFocus(false)
	local Title = TitleCon:CreateLabel(Mission.Name)
	Title:SetFocus(false)
	Title:SetX(TitleCon:GetHorizontalCenter(Title))
	Screen:Paragraph(GUI.GetFont("Elementary_Gothic_Bookhand.ttf", 12), Mission.Description)
	for k, v in pairs(Mission.OptionNames) do
		Screen:CreateLabel(v):OnKey("Enter", "Released", 
		function()
			GUI.SendMessage("Mission", Mission.Options(Data["BigGuy"], World.GetDate())[k])
			GUI.PopMenu()
		end)
	end
	return false
end