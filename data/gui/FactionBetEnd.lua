Menu.Width = 400
Menu.Height = 300
Menu.moveable = true;

function Menu.Init(Menu, Data)
	local ForVote = 0
	local AgainstVote = 0
	local Faction = Data.Faction
	local Str = "The vote has "

	Menu:OnNewChild(Container.Vertical)
	for Idx, Val in pairs(Faction:Active()) do
		if Faction:VoteFor(Val) == true then
			ForVote = ForVote + Faction:GetBet(Val)
		else
			AgainstVote = AgainstVote + Faction:GetBet(Val)
		end
	end
	if ForVote > AgainstVote then
		Str = Str .. " passed!"
	else
		Str = Str .. " failed!"
	end
	Menu:CreateLabel(Str)
	Menu:CreateLabel(tostring(ForVote) .. " power was used to vote for the change.")
	Menu:CreateLabel(tostring(AgainstVote) .. " power was used to vote against the change.")
	Menu:CreateButton("Close",
		function(Widget)
			Menu:Close()
		end)
end

function Menu.Think(Menu)

end
