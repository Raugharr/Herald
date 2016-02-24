Mission.Load {
	Name = "Duel 1",
	Description = "You have lost the duel.",
	Options = {
		{
			Text = "Ok", 
			Trigger = Rule.True(),
			Utility = Mission.Normalize(1, 1)
		}
	},
	Id = "DUEL.1",
	OnTrigger = BigGuy.Kill(Mission.Owner()),
	OnlyTriggered = false,
	MeanTime = 20
}

Mission.Load {
	Name = "Duel 2",
	Description = "You are challanged to a duel by %s.",
	TextFormat = {BigGuy.GetName(Mission.Sender())},
	Options = {
		{
			Text = "Accept the duel",
			Condition = Rule.True(),
			Trigger = Rule.IfThenElse(
			Rule.GreaterThan(
				BigGuy.GetWarfare(Mission.Sender()), 
				BigGuy.GetWarfare(Mission.Owner())
				),
			Rule.Block(
				Mission.CallById("DUEL.1", Mission.Owner()),
				Mission.CallById("DUEL.3", Mission.Sender())
				),
			Rule.Block(
				Mission.CallById("DUEL.1", Mission.Sender()),
				Mission.CallById("DUEL.3", Mission.Owner())
				)
			),
			Utility = Mission.Normalize(1, 1)
		},
		{
			Text = "Look for a champion.",
			Condition = Rule.True(),
			Trigger = Rule.Block(Mission.CallById("DUEL.6", Mission.Owner(), Mission.Sender()), Settlement.BulitinPost("DUEL.4", "DUEL.5", 30, 2)),
			Utility = Mission.Normalize(1, 1)
		}
	},
	Id = "DUEL.2",
	Trigger = {Name = "Crisis", Triggers = {{Type = "WarDeath", OpCode = Mission.Equal, Value = 1}}}, 
	MeanTime = 20
}

Mission.Load {
	Name = "Duel 3",
	Description = "You have won the duel.",
	Options = {
		{
			Text = "Ok", 
			Trigger = Settlement.BulitinPost("DUEL.4", "DUEL.5", 30, 2), --Rule.True(),
			Utility = Mission.Normalize(1, 1)
		}
		},
	Id = "DUEL.3",
	OnlyTriggered = false,
	MeanTime = 20
}

Mission.Load {
	Name = "In need of a champion.",
	Description = "foo is looking for a champion against bar.",
	Options = {
		{
			Text = "Be foo's champing.",
			Trigger = Mission.CallById("DUEL.2", Mission.Owner(), Mission.Sender()),
			Utility = Mission.Normalize(1, 1)
		}
	},
	Id = "DUEL.4",
	OnlyTriggered = false,
	MeanTime = 2
}

Mission.Load {
	Name = "Duel.5",
	Description = "No ones has decided to take your place against bar in the time alloted now you must duel him.",
	Options = {
		{
			Text = "Duel bar.",
			Trigger = Mission.CallById("DUEL.2", Mission.Owner(), Mission.Sender()),
			Utility = Mission.Normalize(1, 1)
		}
	},
	Id = "DUEL.5",
	OnlyTriggered = false
}

Mission.Load {
	Name = "Oponent looks for champion",
	Description = "%s has decided to look for a champion to fight for them in the duel.",
	Options = {
		{
			Text = "Ok",
			Trigger = Rule.True(),
			Utility = Mission.Normalize(1, 1)
		}
	},
	Id = "DUEL.6",
	OnlyTriggered = false
}
