Mission.Load {
	Name = "Harvest feast.",
	Description = "NoDesc.",
	OnTrigger = function(Frame)
		local MenCt = Frame.Owner:GetSettlement():MaleAdults()
		
		if RandomEvent(Math.Probability(Math.RandomVar(0.05), MenCt)) == true then
			Mission.FireEvent("Feast.2", Mission.RandomPerson({Male = true, Adult = true}), Mission.Owner)
		end 
	end,
	Trigger = function(Frame)
		return Frame.Owner == Frame.Owner:GetSettlement():GetLeader()
	end,
	NoMenu = true,
	Event = Event.OnSpring,
	Id = "FEAST.1"
}

Mission.Load {
	Name = "Fight",
	Description = "[From.FirstName] [From.LastName] who is clearly drunk has been telling everyone that you are a sheep shagger.",
	Options = {
		{
			Text = "I will not stand for this insult.",
			Trigger = function(Frame)
				Mission.FireEvent("DUEL.2", Frame.Owner, Frame.From)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Try to reduce the damage.",
			Trigger = function(Frame)
				local MOS = Frame.Owner:SuccessMargin(Stat.Charisma, 100)

				Frame.Owner:ChangePopularity(2.0 - (MOS / 5))
			end,
			AIUtility = function(Frame) end
		}
	},
	Id = "FEAST.2",
	OnlyTriggered = false
}

--Mission.Load {
--	
--}
