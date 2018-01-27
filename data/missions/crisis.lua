local function DebtMission(Frame, Offender)
	if Offender:IsBigGuy() then
		Mission.FireEvent("Debt.1", Offender, Frame.Owner)--]], {{Debt = Frame.Owner:GetSettlement():GetFine(Fine.Murder, Fine.Heavy, Defender)}})	
	end
end
 
Mission.Load {
	Name = "Person murdered",
	Description = "[Defender.FirstName] has accused [Offender.FirstName] of attempted murder.",
--	Declare = {
--		{"Offender", Ptr, Class.Person},
--		{"Defender", Ptr, Class.Person}
--	},
	Options = {
		{
			Text = "Pardon them",
			Trigger = function(Frame)
				local Offender = Frame:GetVar("Offender")
				local Defender = Frame:GetVar("Defender")

				Defender:AddMotivation(Motivation.Murder, Defender)
				--We might want to make a general function that finds a relative and makes them seek revenge.
				for k, Person in ipairs(Offender:GetFamily():People()) do
--[[					if Person:IsMale() == true then
						if Person:IsBigGuy() == false then
							local Guy = BigGuy.Create(Person)

							Guy:AddMotivation(Motivation.Murder, Defender)
							break
						end
					end--]]
				end
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Fine them lightly",
			Trigger = function(Frame)
				local Offender = Frame:GetVar("Offender")
				local Defender = Frame:GetVar("Defender")

			--Event to offender demanding payment. They can refuse if they have no money but then bad stuff happens.
				--GetFine should be a placeholder function that returns an arbatrary value.
				--GetFine's first argument is fine type, followed by severity and then who the fine affected.
				--Mission.FireEvent("Debt.1", Offender, Frame.Owner, {{Debt = Frame.Owner:GetSettlement():GetFine(Fine.Murder, Fine.Light, Defender)}})	
				DebtMission(Frame, Offender)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Fine them normally",
			Trigger = function(Frame)
				local Offender = Frame:GetVar("Offender")
				local Defender = Frame:GetVar("Defender")
			--Event to offender demanding payment. They can refuse if they have no money but then bad stuff happens.
				--Mission.FireEvent("Debt.1", Offender, Frame.Owner, {{Debt = Frame.Owner:GetSettlement():GetFine(Fine.Murder, Fine.Normal, Defender)}})	
				DebtMission(Frame, Offender)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Fine them heavily",
			Trigger = function(Frame)
				local Offender = Frame:GetVar("Offender")
				local Defender = Frame:GetVar("Defender")
			--Event to offender demanding payment. They can refuse if they have no money but then bad stuff happens.
				DebtMission(Frame, Offender)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Cut their hand off",
			Trigger = function(Frame)
			end
			AIUtility = function(Frame) end
		},
		{
			Text = "Put them to death",
			Trigger = function(Frame)
				local Offender = Frame:GetVar("Offender")
				local Defender = Frame:GetVar("Defender")
				local Disaprove = Frame:RandomPerson({BigGuy = true, Target = Offender, Relative = true})
				--local Approve = Frame:RandomPerson({BigGuy = true, Target = Defender, Relative = true})

				for Person in ipairs(Disaprove) do 
					Defender:AddOpinion(Person, Relation.Action.Murder)--Default arguments , Relation.Opinion.Average, Relation.Length.Medium, Relation.Opinion.Average
				end
				Offender.Kill()
				--[[
				if Defender:GetSettlement() == Frame.Owner:GetSettlement() then
					Frame.Owner:AddPopularity(4.0)
				else
					Frame.Owner:AddPopularity(2.0)
				end--]]
			end,
			AIUtility = function(Frame) end
		}
	},
	Crisis = Crisis.Murder,
	Id = "Crisis.1"
}

