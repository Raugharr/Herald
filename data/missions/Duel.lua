Mission.Load {
	Name = "Duel 1",
	Description = "You have lost the duel.",
	Options = {{Text = "Ok", Condition = Rule.True(), Trigger = Rule.True()}},
	Id = "DUEL.1",
	OnTrigger = BigGuy.Kill(Mission.Owner()),
	Trigger = {Name = "Crisis", Type = "WarDeath", OpCode = Mission.Equal, Value = 1},
	MeanTime = 20
}

Mission.Load {
	Name = "Duel 2",
	Description = "You are challanged to a duel.",
	Options = {{Text = "Accept the duel",
		Condition = Rule.True(),
		Trigger = Rule.IfThenElse(
		Rule.GreaterThan(BigGuy.GetWarfare(Mission.Target()), 5),
		Rule.LuaCall(Mission.CallById, "Duel.2", Mission.Target()),
		Rule.LuaCall(Mission.CallById, "Duel.3", Mission.Owner()))
		},
		{Text = "Look for a champion.", Condition = Rule.True(), Trigger = Rule.True()}
	},
	Id = "DUEL.2",
	Trigger = {Name = "Crisis", Type = "WarDeath", OpCode = Mission.Equal, Value = 1}, 
	MeanTime = 20
}

Mission.Load {
	Name = "Duel 3",
	Description = "You have lost the duel.",
	Options = {{Text = "Ok", Condition = Rule.True(), Trigger = Rule.True()}},
	Id = "DUEL.3",
	Trigger = {Name = "Crisis", Type = "WarDeath", OpCode = Mission.Equal, Value = 1},
	MeanTime = 20
}