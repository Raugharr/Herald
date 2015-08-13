FallFeast = {
	Name = "Fall Feast",
	Description = "Time to feast.",
	OptionNames = {"Feed them.", "Don't feed them."},
	Decay = 1,
	Trigger = {IsRuler = false, Month = "October"}
}

function FallFeast.Options(BigGuy, DayStart)
	return 
	{
		{Rule.True(), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, 1), Rule.GreaterThan(World.GetDate(), DayStart + 7), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, -1)},
		{Rule.False(), Rule.True(), Rule.True(), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, -1)}
	}
end