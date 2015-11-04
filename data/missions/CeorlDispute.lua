Mission.Load {
	Name = "Cerol Dispute",
	Description = "You notice two ceorls arguing, when you approch the two to help settle the matter the ceorl on the left says the other has stolen his cattle. The other ceorl rapidly disputes that he had done any such thing.",
	Options = {{Text = "Force the ceorl to give a cow to the other.", Condition = Rule.True(), Trigger = Rule.IfThenElse(
		Rule.LessThan(BigGuy.GetAuthority(Mission.Owner()), 10),
		BigGuy.SetAuthority(Mission.Owner(), -5),
		Rule.Block(BigGuy.SetOpinion(Mission.GetRandomPerson(true), Mission.Owner(), 0, 10), BigGuy.SetOpinion(Mission.GetRandomPerson(true), Mission.Owner(), 0, -10))
		)}, 
		{Text = "Seek divine intervention.", Condition = Rule.GreaterThan(BigGuy.GetPiety(Mission.Owner()), 10), Trigger = Rule.True()}
		},
		Id = "CEROL.1",
		Trigger = {Name = "BigGuy", Type = "Authority", OpCode = Mission.LessThan, Value = 20},
		MeanTime = 2
}