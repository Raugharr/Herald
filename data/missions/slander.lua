Mission.Load {
	Name = "Slander",
	Description = "Lower relations between a target and [Friend.FirstName] [Friend.LastName].",
	OnTrigger = function(Frame)
		local Friend = nil
		
		for Rel in Frame.From:RelationsItr() do
			if Rel:GetOpinion() >= 0 then
				Friend = Rel:BigGuy()
				break
			end
		end
		print(Friend)
		Frame:SetVar("Friend", Friend)	
		if Frame.Owner:OpposedChallange(Frame.From, Stat.Charisma, Stat.Wit) == true then
			Mission.FireEvent("Slander.2", Frame.Owner, Frame.From)
		else
			Mission.FireEvent("Slander.3", Friend, Frame.Owner, {{"Target", Frame.From}})
		end
	end,
	Action = Action.Slander,
	MeanTime = 20,
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
	Description = "Story about how [From.FirstName] was slandering [Target.FirstName].",
	Options = {
		{
			Text = "Don't tell [Target.FirstName].",
			OnTrigger = function(Frame)
				Frame.From:AddOpinion(Frame.Owner, Relation.Action.General, 10)
			end
		},
		{
			Text = "Tell [Target.FirstName].",
			OnTrigger = function(Frame)
				Frame.GetVar("Target"):AddOpinion(Frame.Owner, Relation.Action.General, 10)
			end
		}
	},
	--[[OnTrigger = function(Frame)
		if Frame.Owner:SuccessMargin(Stat.Charisma) < 0 then
			Frame.From:SetOpinion(Frame.Target, Relation.Action.Theft, 10, Relation.Length.Medium, Relation.Opinion.Average)
			Mission.FireEvent("Slander.4", Frame.From, Frame.Owner)
		elseif Random(1, 4) < 2 then
			Mission.FireEvent("Slander.5", Frame.From, Frame.Owner)
		end
	end,--]]
	OnlyTriggered = false,
	MeanTime = 30,
	Id = "Slander.3"
}

Mission.Load {
	Name = "Slander, unsuccessful: target",
	Description = "Tells Owner that [From.FirstName] attempted to slander them.",
	OnlyTriggered = false,
	Id = "Slander.4"
}

Mission.Load {
	Name = "Slander rumors",
	Description = "You hear rumors that there is someone trying to slander your good name.",
	OnlyTriggered = false,
	Id = "Slander.5"
}
