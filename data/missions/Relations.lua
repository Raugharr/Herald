Mission.SetName("Have a feast.")
Mission.SetDesc("Foo")
--[[Mission.AddOption("Sacrifice a cow", Rule.GreaterThan(Rule.LuaCall(Mission.CountAnimal, Mission.Owner(), "Cow"), 1),
	Rule.Block(
	Rule.LuaCall(Mission.SetAuthority, Mission.Owner(), 1),
	Rule.LuaCall(Mission.KillAnimal, Rule.LuaCall(Mission.Owner():GetFamily()), "Cow", 1)
	Rule.LuaCall(Mission.SetOpinion, Rule.LuaCall(Mission.ImproveRelationsTarget, Mission.Owner()), Mission.Owner(), 0, 10)
	)--]]
Mission.AddOption("A cow is to valuable.", Rule.True(), Rule.LuaCall(Mission.CallById, 11111, Mission.Owner()))
Mission.AddOption("A feast is not important right now.", Rule.True(), Rule.True())
Mission.AddTrigger("ImproveRelations", Mission.GreaterThan, 0)
Mission.SetId(11114)
Mission.SetMeanTime(2)

--[[Mission {
	"Name" = "Relations",
	"Description" = "Foo",
	"Options" = {
	{"Ok", Rule.True, Rule.LuaCall(Mission.SetAuthority, Mission.Owner(), -8)}
	},
	"Triggers" = {{"ImproveRelations", Mission.GreaterThan, 0}},
	"Id" = 11114,
	"MeanTime" = 2
}--]]