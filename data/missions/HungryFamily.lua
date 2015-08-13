HungryFamily = {
	Name = "Hungry Family",
	Description = "A family is hungry",
	OptionNames = {"Feed them.", "Don't feed them."},
	Trigger = {IsRuler = false, HasStarvingFamily = true}
}

function HungryFamily.Options(BigGuy, DayStart)
	return 
	{
		{Rule.True(), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, 1), Rule.GreaterThan(World.GetDate(), DayStart + 7), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, -1)},
		{Rule.False(), Rule.True(), Rule.True(), Rule.LuaCall(BigGuy.SetAuthority, BigGuy, -1)}
	}
end