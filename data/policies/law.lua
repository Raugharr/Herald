World.LoadPolicy {
	Name = "Chief power",
	Desc = "How much power the chief can exert.",
	Category = Policy.Law,
	Options = {
		{
			Name = "Minimal Power",
			Desc = "The chief is mearly a figurehead that controls no real power.",
		},
		{
			Name = "Expanded powers",
			Desc = "The chief is allowed to collect taxes in wartime."
		},
	}
}

World.LoadPolicy {
	Name = "Centralization",
	Desc = "How much power the chief of the village can extert.",
	Category = Policy.Law,
	Options = {
		{
			Name = "No Moot",
			Desc = "The ruler of the state has no need for the Moot."
		},
		{
			Name = "Select Moot",
			Desc = "Only certain landowners should be allowed to participate in the Moot."
		},
		{
			Name = "Moot",
			Desc = "Every freeman has the right to attend the Moot."
		}
	}
}
