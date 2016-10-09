--[[Mission.Load {
	Name = "Befriend target.",
	Description = "You will attempt to befriend the target.",
	OnTrigger = function(Frame),
		Mission.FireEvent("Befriend.2", Frame.Owner, Frame.From)
	end,
	OnlyTriggered = false,
	Action = Action.Befriend,
	Id = "Befriend.1"
}--]]

Mission.Load {
	Name = "Befriend target, sucessful.",
	Description = "Story about how you did something to befriend [From.FirstName].",
	OnTrigger = function(Frame)
		Frame.From:SetOpinion(Frame.Owner, Relation.Action.General, 10, Relation.Length.Medium, Relation.Opinion.Average)
		Mission.FireEvent("Befriend.3", Frame.From, Frame.Target)
	end,
	MeanTime = {
		Base = 30 * 2,
		function(Frame)
			return true, (1 - Frame.Owner:GetCharisma() / Stat.Max)
		end
	},
	Action = Action.Befriend,
	Id = "Befriend.2"
}

Mission.Load {
	Name = "Befriend target, sucessful target.",
	Description = "You have a new friend, [From.FirstName].",
	Id = "Befriend.3"
}

--[[Mission.Load {
	Name = "Befriend target, unsucessful.",
	Description = "Story about how you offended [From.FirstName].",
	OnTrigger = function(Frame)
		Frame.From:SetOpinion(Frame.Owner, Relation.Action.General, -10, Relation.Length.Medium, Relation.Opinion.Average)
	end,
	MeanTime = 30 * 2,
	Id = "Befriend.4"
}--]]
