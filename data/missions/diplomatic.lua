local CheatDemands = {
	{"allow him to regain his honor by dueling [Suspect.FirstName]", "Cheater.2"},
	{"force [Suspect.FirstName to give compensation", "Cheater.3"}
}

Mission.Load {
	Name = "Man caught cheating",
	Description = "[Acusser.FirstName] steps up to you declaring that they caught [Suspect.FirstName] and demands that you [Demand].",
	OnTrigger = function(Frame)
		local AcusserLocation = World.SettlementsInRange(Frame.Owner:GetFamily():GetSettlement(), 35)
		local People = nil 
		local Demand = Random(1, #CheatDemands)

		AcusserLocation = AcusserLocation[Random(1, #AcusserLocation)]
		if AcusserLocation == Frame.Owner:GetFamily():GetSettlement() then
			Frame:RandomPerson({Adult = true, BigGuy = false, Married = true, Count = 2})
			Frame:SetVar("Acusser", People[1])
			Frame:SetVar("Suspect", People[2])
		else
			Frame:SetVar("Acusser", Frame:RandomPerson({Adult = true, BigGuy = false, Married = true, Count = 1}, AcusserLocation))
			Frame:SetVar("Suspect", Frame:RandomPerson({Adult = true, BigGuy = false, Married = true, Count = 1}))
		end
		if #People ~= 2 then
			return false
		end
		Frame:SetVar("Demand", CheatDemands[Demand][1])
		return true
	end,
	Options = {
		{
			Text = "Allow [Acusser.FirstName] to duel [Suspect.FirstName]",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Force [Suspect.FirstName] to give [Acusser.FirstName] compensation.",
			Trigger = function(Frame) end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	MeanTime = 100 * 12,
	Id = "Cheater.1"
}
