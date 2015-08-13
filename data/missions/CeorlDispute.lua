HungryFamily = {
	Name = "Cerol Dispute",
	Description = "You notice two ceorls arguing, when you approch the two to help settle the matter the ceorl on the left says the other has stolen his cattle. The other ceorl rapidly disputes that he had done any such thing.",
	OptionNames = {"Do nothing", "Force the ceorl to give a cow to the other."},
	Trigger = {Authority = {"LessThan", 20}}
}

function HungryFamily.Options(BigGuy, DayStart)
	return 
	{
		{Rule.LessThan(Rule.LuaCall(BigGuy.GetAuthority, BigGuy), 25), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, -8)}
		{Rule.True(), Rule.IfThenElse(
		Rule.LessThan(Rule.LuaCall(BigGuy.GetAuthority, BigGuy), 10), 
		Rule.Call(BigGuy.SetAuthority, -5),
		Rule.DoBlock(Rule.LuaCall(BigGuy.SetOpinion, Mission.GetRandomPerson(true, true, true), BigGuy), Rule.LuaCall(BigGuy.SetOpinion, Mission.GetRandomPerson(true, true, true)
	}
end

Mission.SetName("Cerol Dispute")
Mission.SetDesc("You notice two ceorls arguing, when you approch the two to help settle the matter the ceorl on the left says the other has stolen his cattle. The other ceorl rapidly disputes that he had done any such thing.")
Mission.AddOption("Do Nothing", Rule.LessThan(Rule.LuaCall(BigGuy.GetAuthority, BigGuy), 25), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, -8))