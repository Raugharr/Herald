Mission.Load {
	Name = "Duel 1",
	Description = "You have lost the duel and have been kiled by [From.FirstName].",
	Id = "Duel.1",
	OnTrigger = function(Frame)
		Mission.Owner:Kill()
	end,
	OnlyTriggered = false
}

Mission.Load {
	Name = "Duel 2",
	Description = "You are challanged to a duel by %s.",
	Options = {
		{
			Text = "Accept the duel",
			Trigger = function(Frame)
				if Frame.From:OpposedChallange(Frame.Owner, Stat.Combat) >= 0 then
					Mission.FireEvent("Duel.3", Mission.From, Mission.Owner)
					Mission.FireEvent("Duel.1", Mission.Owner, Mission.Sender)
				else
					Mission.FireEvent("Duel.3", Mission.Owner, Mission.Sender)
					Mission.FireEvent("Duel.1", Mission.From, Mission.Owner)
				end
			end,
			AIUtility = function(Frame) end 
		},
		{
			Text = "Look for a champion.",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.6", Mission.From, Mission.Owner)
				Settlement.BulitinPost("Duel.4", "DUEL.5", 30, 2)
			end,
			AIUtility = function(Frame) end
		}
	},
	Id = "Duel.2",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Duel 3",
	Description = "You have won the duel and have slain [From.FirstName].",
	Id = "Duel.3",
	OnlyTriggered = false
}

Mission.Load {
	Name = "In need of a champion.",
	Description = "foo is looking for a champion against bar.",
	Options = {
		{
			Text = "Be foo's champing.",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.2", Mission.Owner, Mission.Sender)
			end,
			AIUtility = function(Frame) end
		}
	},
	Id = "Duel.4",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Duel.5",
	Description = "No ones has decided to take your place against bar in the time alloted now you must duel him.",
	Options = {
		{
			Text = "Duel bar.",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.2", Mission.Owner, Mission.Sender)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Run away.",
			AIUtility = function(Frame) end
		}
	},
	Id = "Duel.5",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Oponent looks for champion",
	Description = "[From.FirstName] has decided to look for a champion to fight for them in the duel.",
	Id = "Duel.6",
	OnlyTriggered = false
}

