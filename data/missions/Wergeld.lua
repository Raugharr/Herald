Mission.Load {
	Name = "Give wergeld.",
	Description = "You have given a cow in payment of wergeld.",
	Options = {
		{
			Text = "Ok",
			Trigger = Rule.True(),
			Utility = Mission.Normalize(0, 1)
		}
	},
	OnTrigger = Family.TakeAnimal(BigGuy.GetFamily(Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner()))), BigGuy.GetFamily(Mission.Target()), "Cow", 1),
	OnlyTriggered = true,
	Id = "WERGEL.1"
}

Mission.Load {
	Name = "Wergeld",
	Description = "You have been demanded to give another wergeld.",
	Options = {
		{
			Text = "Give a cow.",
			Trigger = Mission.CallById("WERGEL.1", Mission.Target(), Mission.Owner()),
			Condition = Rule.GreaterThan(Mission.Data("CowCt"), 0),
			Utility = Mission.Normalize(Mission.Data("CowCt"), Mission.Data("CowCt"))
		},
		{
			Text = "Give him nothing.",
			Trigger = Rule.True(),
			Utility = Mission.Normalize(Rule.GreaterThan(BigGuy.GetRelation(Mission.Owner(), Mission.Target()),  BigGuy.Like))
		}
	},
	OnTrigger = Mission.AddData("CowCt", Family.CountAnimal(BigGuy.GetFamily(Mission.Owner()), "Cow")),
	OnlyTriggered = true,
	Id = "WERGEL.2"
}

Mission.Load {
	Name = "Wergeld Rejected.",
	Description = "Your demand of wergeld has been rejected by the king.",
	Options = {
		{
			Text = "Accept the decision.",
			Trigger = Rule.True(),
			Utility = Mission.Normalize(1, 2)
		},
		{
			Text = "Duel them instead.",
			Trigger = Mission.CallById("DUEL.2", Mission.Target(), Mission.Owner()),
			Utility = Mission.Normalize(0, 1)
		},
		{
			Text = "Duel the king.",
			Trigger = Mission.CallById("DUEL.2", Settlement.GetLeader(BigGuy.GetSettlement(Mission.Target())), Mission.Owner()),
			Utility = Mission.Normalize(0, 1)
		}
	},
	OnlyTriggered = true,
	Id = "WERGEL.3"
}