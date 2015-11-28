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
	Description = "You are challanged to a duel.",
	Options = {
		{
			Text = "Accept the duel",
			Condition = Rule.True(),
			Trigger = Rule.IfThenElse(
			Rule.GreaterThan(BigGuy.GetWarfare(Mission.Target()), BigGuy.GetWarfare(Mission.Owner())),
			Rule.Block(Rule.LuaCall(Mission.CallById, "DUEL.1", Mission.Target()),
				Rule.LuaCall(Mission.CallById, "DUEL.3", Mission.Owner())),
			Rule.Block(Rule.LuaCall(Mission.CallById, "DUEL.3", Mission.Target()),
				Rule.LuaCall(Mission.CallById, "DUEL.1", Mission.Owner()))),
			Utility = Mission.Normalize(1, 1)
		},
		{
			Text = "Look for a champion.",
			Condition = Rule.True(),
			Trigger = Rule.True(),
			Utility = Mission.Normalize(1, 1)
		}
	},
	Id = "DUEL.2",
	Trigger = {Name = "Crisis", Type = "WarDeath", OpCode = Mission.Equal, Value = 1}, 
	MeanTime = 20
}

Mission.Load {
	Name = "Duel 3",
	Description = "You have lost the duel.",
	Options = {
		{
			Text = "Ok", 
			Trigger = Rule.True(),
			Utility = Mission.Normalize(1, 1)
		}
		},
	Id = "DUEL.3",
	OnlyTriggered = false,
	MeanTime = 20
}