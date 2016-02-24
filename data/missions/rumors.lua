Mission.Load {
	Name = "Viscious Rumor", 
	Description = "A cerol comes up to you declaring that another has stolen his cow and tells you of his intention to duel him.",
	Options = {
		{
			Text = "Spread the rumor.",
			Condition = Rule.True(),
			Trigger = Rule.True(),--Rule.ForEach(BigGuy.FriendList(Mission.Owner), Mission.CallById("RUMOR.2", Mission.Owner(), Rule.Iterator())),
			Utility = Mission.Normalize(1, 1)
		},
		{
			Text = "Destroy their reputation in a different way.",
			Condition = Rule.True(),
			Trigger = Rule.True(),
			Utility = Mission.Normalize(0, 1)
		}
	},
	Trigger = {Name = "Event", Triggers = {"SabatogeRelations"}},
	Id = "RUMOR.1",
	MeanTime = 30
}

Mission.Load {
	Name = "Viscious Rumor", 
	Description = "A cerol comes up to you declaring that another has stolen his cow and tells you of his intention to duel him.",
	Options = {
		{
			Text = "Continue to spread the rumor.",
			Condition = Rule.True(),
			Trigger = Rule.True(),--[[Rule.Block(
				Rule.ForEach(BigGuy.FriendList(Mission.Owner), Mission.CallById("RUMOR.2", Mission.Owner()), Rule.Iterator()),
				BigGuy.SetOpinion(Mission.Owner(), Mission.Sender(), BigGuy.Relation.Small, 10),
				BigGuy.SetOpinion(Mission.Owner(), Mission.Target(), BigGuy.Relation.Small, -10)),--]]
			Utility = Mission.Normalize(1, 1)
		},
		{
			Text = "Refuse to spread rumors about %s.",
			--TextFormat = {BigGuy.GetName(Mission.Sender())},
			Trigger = Mission.CallById("RUMOR.3", Mission.Sender(), Mission.Owner()),
			Utility = Mission.Normalize(0, 1)
		},
		{
			Text = "Inform %s of the rumor.",
			TextFormat = Rule.True(),--{BigGuy.GetName(Mission.Sender())},
			Trigger = Rule.Block(
				BigGuy.SetOpinion(Mission.Owner(), Mission.Sender(), 1--[[ BigGuy.Relation.Average--]], -20),
				BigGuy.SetOpinion(Mission.Owner(), Mission.Sender(), 1--[[ BigGuy.Relation.Average--]], 10)),
			Utility = Mission.Normalize(0, 1)
		},
	},
	OnlyTriggered = true,
	Id = "RUMOR.2",
	MeanTime = 30
}

Mission.Load {
	Name = "Viscious Rumor",
	Description = "%s has refused to spread the rumor.",
	Options = {
		{
			Text = "Rebuke them.",
			Trigger = BigGuy.SetOpinion(Mission.Owner(), Mission.Sender(), 1--[[BigGuy.Relation.Average--]], -15),
			Utility = Mission.Normalize(1, 1)
		},
		{
			Text = "Insult %s.",
			Trigger = Mission.CallById("INSLT.1", Mission.Sender(), Mission.Owner()),
			Utility = Mission.Normalize(0, 1)
		}
	},
	OnlyTriggered = true,
	Id = "RUMOR.3",
	MeanTime = 30
}
