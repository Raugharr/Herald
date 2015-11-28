Mission.Load {
	Name = "Have a feast.",
	Description = "Foo",
	Options = {
		{
			Text = "A cow is to valuable.",
			Condition = Rule.True(),
			Trigger = Rule.True(),
			Utility = Mission.Normalize(1, 1)
		},
		{
			Text = "A feast is not important right now",
			Condition = Rule.True(),
			Trigger = Rule.True(),
			Utility = Mission.Normalize(0, 1)
		}
	},
	Trigger = {Name = "BigGuy", Type = "ImproveRelations", OpCode = Mission.GreaterThan, Value = 0},
	MeanTime = 2,
	Id = "REL.1"
}