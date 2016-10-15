function DisplayPlotMenu(Menu, Left, Right)
	local Actions = {}
	local Plot = Menu.Plot

	Left:Clear()
	Left:CreateLabel("Current threat: " .. tostring(Plot:GetThreat()))
	Left:CreateLabel("Warscore: " .. tostring(Plot:GetScore()))
	if Plot:HasStarted() == false then
		Left:CreateButton("Start plot.",
			function()
				Plot:Start(true)
			end)
	end
	DisplayPlotters(Menu, Left, Right)
	DisplayPlotDefenders(Menu, Left, Right)
	Left:CreateButton("Show History",
		function()
			Right:Clear()
			for i, Action in ipairs(Plot:PrevMonthActions()) do
				Right:CreateLabel(Action:Describe())	
			end
		end)
	Menu.CbtLbl = Left:CreateLabel("Combat Abilities")
	Actions[#Actions + 1] = Left:CreateButton("Double Damage",
		function()
			Plot:AddAction(Plot.DoubleDamage,  World.GetPlayer(), nil)			
		end)
	Actions[#Actions + 1] = Left:CreateButton("Double Attack",
		function()
			Plot:AddAction(Plot.DoubleAttack,  World.GetPlayer(), nil)			
		end)
	Left:CreateLabel("Wit Abilities")
	Actions[#Actions + 1] = Left:CreateButton("Prevent Damage",
		function()
			Plot:AddAction(Plot.Prevent,  World.GetPlayer(), Menu.Guy)			
		end)
	Left:CreateLabel("Charisma Abilities")
	Left:CreateButton("Back", function()
		DisplayViewPerson(Menu, Left, Right)
	end)
end

function DisplayPlotDefenders(Menu, Left, Right)
	local List = {}
	local PersonList = {} 

	Right:CreateLabel("Defenders")
	List = FillList(Menu.Plot:Defenders().Front, Menu.Plot:Defenders())
	for k, v in ipairs(List) do
		PersonList[#PersonList + 1] = v:GetPerson()
	end
	FillPersonTable(CreatePersonTable(Right, #PersonList), PersonList, Menu.Plot:Defenders()) 
end

function DisplayPlotters(Menu, Left, Right)
	local List = {}
	local PersonList = {} 

	Right:CreateLabel("Plotters")
	List = FillList(Menu.Plot:Plotters().Front, Menu.Plot:Plotters())
	for k, v in ipairs(List) do
		PersonList[#PersonList + 1] = v:GetPerson()
	end
	FillPersonTable(CreatePersonTable(Right, #PersonList), PersonList, Menu.Plot:Plotters())
end

