Mission.Load {
	Name = "Duel 1",
	Description = "You have lost the duel and have been kiled by [From.FirstName].",
	OnTrigger = function(Frame)
		Frame.Owner:Kill()
	end,
	OnlyTriggered = false,
	Id = "Duel.1"
}

Mission.Load {
	Name = "Duel 2",
	Description = "You are challanged to a duel by [From.FirstName].",
	Options = {
		{
			Text = "Accept the duel",
			Trigger = function(Frame)
				if Frame.From:OpposedChallange(Frame.Owner, Stat.Combat) >= 0 then
					Mission.FireEvent("Duel.9", Frame.Owner, Frame.From)
				else
					Mission.FireEvent("Duel.9", Frame.From, Frame.Owner)
				end
			end,
			AIUtility = function(Frame) return 1 end 
		}--,
		--Comented out for debugging.
		--[[{
			Text = "Look for a champion.",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.6", Frame.From, Frame.Owner)
				Frame.Owner:BulletinPost("Duel.4", "Duel.5", 30, 2)
			end,
			AIUtility = function(Frame) end
		}--]]
	},
	Id = "Duel.2",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Duel 3",
	Description = "You have won the duel and have defeated [From.FirstName].",
	Options = {
		{
			Text = "Kill [From.ProObj]",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.1", Frame.From, Frame.Owner)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Show mercy towards [From.ProObj]",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.8")
			end,
			AIUtility = function(Frame) end,
			Condition = function(Frame)
				if Random(0, 1) == 0 then
					return false
				end
				if Frame.Owner:HasTrait("Cruel") then
					return false
				end
				return true
			end
		}
	},
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
				Mission.FireEvent("Duel.2", Frame.Owner, Mission.Sender)
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
			Text = "Duel [From.FirstName].",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.2", Frame.Owner, Mission.Sender)
			end,
			AIUtility = function(Frame) end
		},
		{
			Text = "Run away.",
			Trigger = function(Frame)
			end,
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

Mission.Load {
	Name = "Challange made",
	Description = "You have challanged [From.FirstName] to a duel.",
	Id = "Duel.7",
	OnTrigger = function(Frame)
		Mission.FireEvent("Duel.2", Frame.From, Frame.Owner)
	end,
	OnlyTriggered = false,
	Action = Action.Duel
}

Mission.Load {
	Name = "Life spared",
	Description = "Although [From.FirstName] has defeated you in the duel [From.ProSub] had decided to spare your life.",
	Id = "Duel.8",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Loosing duel",
	Description = "Although the fight is not over you are clearly loosing the duel",
	Options = {
		{
			Text = "Continue to fight",
			Trigger = function(Frame)
				Mission.FireEvent("Duel.3", Frame.From, Frame.Owner)	
			end,
			AIUtility = function(Frame)
			end
		},
		{
			Text = "Beg for your life",
			Trigger = function(Frame)
			end,
			AIUtility = function(Frame) end
		}
	},
	OnlyTriggered = false,
	Id = "Duel.9"
}
