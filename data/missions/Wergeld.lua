--[[Mission.Load {
	Name = "Give wergeld.",
	Description = "You have given a cow in payment of wergeld.",
	Options = {
		{
			Text = "Ok",
			Trigger = Rule.True(),
			AIUtility = Mission.Normalize(0, 1)
		}
	},
	OnTrigger = Family.TakeAnimal(BigGuy.GetFamily(Settlement.GetLeader(BigGuy.GetSettlement(Mission.Owner()))), BigGuy.GetFamily(Mission.Sender()), "Cow", 1),
	OnlyTriggered = true,
	Id = "WERGEL.1"
}

Mission.Load {
	Name = "Wergeld",
	Description = "You have been demanded to give another wergeld.",
	Options = {
		{
			Text = "Give a cow.",
			Trigger = Mission.FireEvent("WERGEL.1", Mission.Sender(), Mission.Owner()),
			Condition = Rule.GreaterThan(Mission.Var("CowCt"), 0),
			AIUtility = Mission.Normalize(Mission.Var("CowCt"), Mission.Var("CowCt"))
		},
		{
			Text = "Give him nothing.",
			Trigger = Rule.True(),
			AIUtility = Mission.Normalize(Rule.GreaterThan(BigGuy.GetRelation(Mission.Owner(), Mission.Sender()),  BigGuy.Like))
		}
	},
	OnTrigger = Mission.SetVar("CowCt", Family.CountAnimal(BigGuy.GetFamily(Mission.Owner()), "Cow")),
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
			AIUtility = Mission.Normalize(1, 2)
		},
		{
			Text = "Duel them instead.",
			Trigger = Mission.FireEvent("DUEL.2", Mission.Sender(), Mission.Owner()),
			AIUtility = Mission.Normalize(0, 1)
		},
		{
			Text = "Duel the king.",
			Trigger = Mission.FireEvent("DUEL.2", Settlement.GetLeader(BigGuy.GetSettlement(Mission.Sender())), Mission.Owner()),
			AIUtility = Mission.Normalize(0, 1)
		}
	},
	OnlyTriggered = true,
	Id = "WERGEL.3"
}--]]
