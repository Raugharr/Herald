--[[World.LoadPolicy {
	Name = "Irregular Infantry",
	Desc = "How your irregular infantry are armed",
	Category = Policy.Military,
	Options = {
		{Name = "Arms", 
			{"Spear"}, 
			{"Seax"}, 
			{"Seax and javalin"}
			{"Spear and javalin"},
		},
		{Name = "Armor", 
			{"None", Fyrd = Policy.Dislike},
			{"Gambeson"}, 
			{"Partial Leather"},
			{"Full leather", Fyrd = Policy.Like}
		},
		{Name = "Shield",
			{"None", Fyrd = Policy.Hate}, 
			{"Buckler"}, 
			{"Round shield", Policy.Like}
		}
		}
}

World.LoadPolicy {
	Name = "Regular Infantry",
	Desc = "How well your regular infantry are armed",
	Category = Policy.Military,
	Options = {
		{Name = "Arms", "Spear", {"Spear and javalin"}, {"Sword"}, {"Axe"}},
		{Name = "Secondary arm", {"Seax"}, {"Spear"}, {"Sword"}},
		{Name = "Armor", 
			{"None", Warrior = Policy.Hate, Memory = Policy.Forever},
			{"Gambeson"},
			{"Full leather", Warrior = Policy.Like, Memory = Policy.Forever},
			{"Mail", Warrior = Policy.Love, Memory = Policy.Forever}
		},
		{Name = "Shield", 
			{"None", Warrior = Policy.Hate, Memory = Policy.Forever}, 
			{"Buckler", Warrior = Policy.Dislike, Memory = Policy.Forever}, 
			{"Round shield", Warrior = Policy.Love, Memory = Policy.Forever}}
		}
}--]]

World.LoadPolicy {
	Name = "Warrior organization",
	Desc = "Determines the power that retinues have.",
	Category = Policy.Military,
	Options = {
		{
			Name = "Professional Army",
			Desc = "With the retinue disbanded we can now focus on the creation of a professional force."
		},
		{
			Name = "Outlaw Retinues",
			Desc = "Retinues have proved to give a single person to much power and must be put to an end."
		},
		{
			Name = "Warriors Privledged",
			Desc = "Warriors are lead by the warlord who is expected to give warrios gifts in return for their service."
		},
		{
			Name = "Landed Retinue",
			Desc = "Warriors in the retinue have expanded privledges and now expect to be given estates."
		}
	}
}

World.LoadPolicy {
	Name = "Fyrd organization",
	Desc = "The structure of the states militia.",
	Category = Policy.Military,
	Options = {
		{
			Name = "No fyrd",
			Desc = "The warriors are powerful enough without the need of a fyrd."
		},
		{
			Name = "Fyrd",
			Desc = "Every able man is required to serve in the fyrd."
		},
		{
			Name = "Select fyrd",
			Desc = "Allow only wealthy freemen to serve in the fyrd."
		}
	}
}

World.LoadPolicy {
	Name = "Warlord power",
	Desc = "How much power the warlord has in everyday affairs.",
	Category = Policy.Military,
	Options = {
		{
			Name = "Powerless warlord",
			Desc = "The warlord should only serve to control the army and have no power in the state's politics."
		},
		{
			Name = "Respected warlord",
			Desc = "The warlord now has expanded powers and has extra power in the government"
		},
	}
}

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
			Name = "Moot",
			Desc = "Every freeman has the right to attend the Moot."
		},
		{
			Name = "Select Moot",
			Desc = "Only certain landowners should be allowed to participate in the Moot."
		}
	}
}
