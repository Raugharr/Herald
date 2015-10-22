Mission.SetName("Cerol Dispute")
Mission.SetDesc("You notice two ceorls arguing, when you approch the two to help settle the matter the ceorl on the left says the other has stolen his cattle. The other ceorl rapidly disputes that he had done any such thing.")
Mission.AddOption("Do Nothing", Rule.LessThan(Rule.LuaCall(Mission.GetAuthority, Mission.Owner()), 25), Rule.LuaCall(Mission.SetAuthority, Mission.Owner(), -8))
Mission.AddOption("Force the ceorl to give a cow to the other.", Rule.True(), Rule.IfThenElse(
		Rule.LessThan(Rule.LuaCall(Mission.GetAuthority, Mission.Owner()), 10),
		Rule.LuaCall(Mission.SetAuthority, Mission.Owner(), -5),
		Rule.Block(Rule.LuaCall(Mission.SetOpinion, Mission.GetRandomPerson(true), Mission.Owner(), 0, 10), Rule.LuaCall(Mission.SetOpinion, Mission.GetRandomPerson(true), Mission.Owner(), 0, -10))
		))
--[[Mission.AddOption("Give a cow to the cerol as compensation.", Rule.GreaterThan(LuaCall(Mission.CountAnimal, Mission.Owner(), "Cow"), 0),
		Rule.Block(Mission.SetTarget(Mission.GetRandomPerson(true)), Rule.LuaCall(Mission.GiveAnimal, Mission.Owner(), Mission.GetTarget(), "Cow"), 
		Rule.LuaCall(Mission.SetOpinion, Mission.GetTarget(), Mission.Owner(), 0, 15), Rule.LuaCall(Mission.SetAuthority, Mission.Owner(), 10))
		)--]]
Mission.AddTrigger("Authority", Mission.LessThan, 20)
Mission.SetId(11111)
Mission.SetMeanTime(2)