GovernmentMenu = { }

function GovernmentMenu.Init(Width, Height, Data)
	local Screen = GUI.VerticalContainer(0, 0, 512, Height, 0, {0, 0, 0, 0})
	local VoteCont = GUI.VerticalContainer(0, 0, 512, 100, 0, {0, 0, 0, 0}, Screen)
	local Government = Data
	local Reform = Government:GetReform()
	
	World.Pause(true)
	local TitleCon = GUI.HorizontalContainer(0, 0, Width, 30, 0, {0, 0, 0, 0}, Screen)
	TitleCon:SetFocus(false)
	local Title = TitleCon:CreateLabel("Government")
	Title:SetFocus(false)
	Title:SetX(TitleCon:GetHorizontalCenter(Title))
	--Screen:Paragraph(GUI.GetFont("Elementary_Gothic_Bookhand.ttf", 12), Mission.Description)
	--Government:PossibleReforms():Front():Data():GetName()
	VoteCont:SetFocus(false)
	if Reform ~= nil then
		GovernmentMenu.VoteLabel = VoteCont:CreateLabel(Reform:GetVotes() .. " votes for passing  out of " .. Reform:GetMaxVotes() .. " total votes.")
	end
	
	Screen:CreateLabel(Government:Rule() .. " " .. Government:Structure() .. " " .. Government:Type()):SetFocus(false)
	for v in Government:PossibleReforms():Front() do
		Screen:CreateLabel(v:GetName()):OnKey("Enter", "Released",
			function()
				Government:PassReform(v)
			 end)
	end
	Screen:CreateLabel("Back"):OnKey("Enter", "Released",
		function()
			GUI.PopMenu()
		end)
	return false
end

function GovernmentMenu.Think(Menu)

end