Mission.Load {
	Name = "Slander",
	Description = "Lower relations between a target and a random friend.",
	Trigger = function(Frame)
		local Friend = nil
		
		for Rel in Frame.From:GetRelationList() do
			if Rel:GetOpinion() >= 0 then
				Friend = Rel:BigGuy()
				break
			end
		end
		Frame.SetVar("Friend", Friend)	
		if Frame.Owner:OpposedChallange(Frame.From, Stat.Charisma, Stat.Wit) == true then
			Frame.FireEvent("Slander.2", Frame.Owner, Frame.From)
		else
			Frame.FireEvent("Slander.3", Frame.Owner, Frame.From)
		end
	end,
	Action = Action.Slander,
	OnlyTriggered = false,
	Id = "Slander.1"
}

Mission.Load {
	Name = "Slander, successful.",
	Description = "Story about how [From.FirstName] and random friend had a falling out.",
	--Options = {
--
--	}
	OnTrigger = function(Frame)
		--change relations with var("Friend") and Frame.From
		Frame.From:SetOpinion(Frame.Owner, Relation.Action.Theft, 10, Relation.Length.Medium, Relation.Opinion.Average)
	end,
	OnlyTriggered = false,
	MeanTime = 30 * 2,
	Id = "Slander.2"
}

Mission.Load {
	Name = "Slander, unsuccessful.",
	Description = "Story about how [From.FirstName] found out you were slandering him.",
	OnTrigger = function(Frame)
		Frame.From:SetOpinion(Frame.Target, Relation.Action.Theft, 10, Relation.Length.Medium, Relation.Opinion.Average)
		Frame.FireEvent("Slander.4", Frame.From, Frame.Owner)
	end,
	OnlyTriggered = false,
	MeanTime = 30 * 2,
	Id = "Slander.3"
}

Mission.Load {
	Name = "Slander, unsuccessful: target",
	Description = "Tells Owner that [From.FirstName] attempted to slander them.",
	OnlyTriggered = false,
	Id = "Slander.4"
}
