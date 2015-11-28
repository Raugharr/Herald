Mission.Load {
	Name = "Cerol Dispute", 
	Description = "A cerol comes up to you declaring that another has stolen his cow and tells you of his intention to duel him.",
	Options = {
		{
			Text = "Allow the duel.",
			Condition = Rule.True(),
			Trigger = Mission.CallById("DUEL.2", Mission.Owner(), Mission.Data("Cerol")),
			Utility = Mission.Normalize(0, 1)
		},
		{
			Text = "Force the cerol to take weregeld instead.",
			Condition = Rule.True(),
			Trigger = Rule.Block(
				BigGuy.SetAuthority(Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner())), 10), 
				BigGuy.SetOpinion(Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner()), 0, -15)),
				Mission.CallById("WERGEL.1", Mission.Owner(), Mission.Data("Cerol"))),
				Utility = Mission.Normalize(0, 1)
		},
		{
			Text = "Refuse the cerol the right to deul.",
			Condition = Rule.True(),
			Trigger = Rule.Block(BigGuy.SetAuthority(Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner())), 20),
				BigGuy.SetOpinion(Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner()), 0, -20))),
			Utility = Mission.Normalize(0, 1)
		}
	},
	OnTrigger = Mission.AddData("Cerol", Mission.GetRandomPerson(true), "BigGuy"),
	OnlyTriggered = true,
	Id = "CEROL.1",
	MeanTime = 30
}

Mission.Load {
	Name = "Cerol Dispute",
	Description = "You notice two ceorls arguing, when you approch the two to help settle the matter the ceorl on the left says the other has stolen his cattle. The other ceorl rapidly disputes that he had done any such thing.",
	Options = {{Text = "Force the ceorl to give a cow to the other.", 
		Condition = Rule.True(), 
		Trigger = Rule.IfThenElse(
		Rule.LessThan(BigGuy.GetAuthority(Mission.Owner()), 10),
		BigGuy.SetAuthority(Mission.Owner(), -5),
		Rule.Block(BigGuy.SetOpinion(Mission.GetRandomPerson(true), Mission.Owner(), 0, 10), BigGuy.SetOpinion(Mission.GetRandomPerson(true), Mission.Owner(), 0, -10))),
		Utility = Mission.Normalize(0, 1)
		}, 
		{
			Text = "Seek divine intervention.",
			Condition = Rule.GreaterThan(BigGuy.GetPiety(Mission.Owner()), 10),
			Trigger = Rule.True(),
			Utility = Mission.Normalize(0, 1)
		}
	},
	Id = "CEROL.2",
	OnlyTriggered = true,
	MeanTime = 2
}

Mission.Load {
	Name = "Cerol Dispute",
	Description = "Another cerol has taken your cow.",
	Options = {
		{
			Text = "Duel the ceorl.",
			Condition = Rule.True(),
			Trigger = Mission.CallById("DUEL.2", Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner()), Mission.Owner()), Mission.Owner()),
			Utility = Rule.Block(Rule.IfThenElse(Rule.GreaterThan(BigGuy.GetRelation(Mission.Owner(), Mission.Data("Cerol")),  BigGuy.Like),
					Mission.Normalize(0, 1),
					Mission.Normalize(BigGuy.GetWarfare(Mission.Owner()), BigGuy.GetWarfare(Mission.Data("Cerol")))
				))
		},
		{
			Text = "Demand weregeld.",
			Condition = Rule.True(),
			Trigger = Mission.CallById("CEROL.4", Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner())), Mission.Owner(), Mission.Owner()),
			Utility = Mission.Normalize(0, 1)
		},
		{
			Text = "Do nothing",
			Condition = Rule.GreaterThan(BigGuy.GetRelation(Mission.Data("Cerol"), Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner()))), 0),
			Trigger = Rule.Block(BigGuy.SetOpinion(Mission.Owner(), Mission.Data("Cerol"), 0, -15), Mission.CallById("CEROL.5", BigGuy.GetSettlement(Mission.Owner()))),
			Utility = Mission.Normalize(0, 1)
		}
	},
	OnTrigger = Mission.AddData("Cerol", Mission.GetRandomPerson(true), "BigGuy"),
	Trigger = {Name = "BigGuy", Type = "Authority", OpCode = Mission.LessThan, Value = 20},
	--Trigger = {Name = "BigGuy", {Type = "Authority", OpCode = Mission.LessThan, Value = 20}, {Type = "IsLeader", OpCode = Mission.Equal, Value = 0}},
	Id = "CEROL.3",
	MeanTime = 2
}

Mission.Load {
	Name = "Cerol Dispute",
	Description = "One of your men is demanding wergeld from another.",
	Options = {
		{
			Text = "Permit it",
			Trigger = Mission.CallById("WERGEL.2", Mission.Target(), Mission.Owner()),
			Utility = Mission.Normalize(0, 1)
		},
		{
			Text = "Forbit it",
			Trigger = Mission.CallById("WERGEL.3", Mission.Target(), Mission.Owner()),
			Utility = Mission.Normalize(0, 1)
		}
	},
	Id = "CEROL.4",
	OnlyTriggered = true
}

Mission.Load {
	Name = "Cerol Dispute",
	Description = "Your man has listened to your demand.",
	Options = {
		{
			Text = "Ok",
			Trigger = Rule.True(),
			Utility = Mission.Normalize(0, 1)
		}
	},
	Id = "CEROL.5",
	OnlyTriggered = true
}
