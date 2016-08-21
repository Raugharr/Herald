function DisplayPlotMenu(Menu, Left, Right)
	local Actions = {}

	Left:Clear()
	Left:CreateLabel("Current threat: " .. tostring(Menu.Plot:GetThreat()))
	Left:CreateLabel("Warscore: " .. tostring(Menu.Plot:GetScore()))
	Left:CreateButton("Show Plotters", 
		function()
			DisplayPlotters(Menu, Left, Right)
		end)
	Left:CreateButton("Show Defenders", 
		function()
			DisplayPlotDefenders(Menu, Left, Right)
		end)
	Left:CreateButton("Show History",
		function()
			Right:Clear()
			for i, Action in ipairs(Menu.Plot:PrevMonthActions()) do
				Right:CreateLabel(Action:Describe())	
			end
		end)
	Actions[#Actions + 1] = Left:CreateButton("Prevent Damage",
		function()
			Menu.Plot:AddAction(Plot.Prevent,  World.GetPlayer(), Menu.Guy)			
		end)
	Actions[#Actions + 1] = Left:CreateButton("Double Damage",
		function()
			Menu.Plot:AddAction(Plot.DoubleDamage,  World.GetPlayer(), nil)			
		end)
	Actions[#Actions + 1] = Left:CreateButton("Double Attack",
		function()
			Menu.Plot:AddAction(Plot.DoubleAttack,  World.GetPlayer(), nil)			
		end)
	Left:CreateButton("Back", function()
		DisplayViewPerson(Menu, Left, Right)
	end)
end


