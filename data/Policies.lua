Policy.Load {
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

Policy.Load {
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
}

Policy.Load {
	Name = "Property Tax",
	Desc = "",
	Category = Policy.Economy,
	Options = {
		{"Low", Policy.Like, Memory = Policy.Forever},
		{"Medium", Policy.Unfavored, Memory = Policy.Forever},
		{"High", Policy.Dislike, Memory = Policy.Forever}
	}
}

Policy.Load {
	Name = "Crop Tax",
	Desc "How much of each harvest must be given as tax.",
	Category = Policy.Economy,
	Options = {
		{"Low", Peasant = Policy.Unfavored, Memory = Policy.Forever},
		{"Medium", Peasant = Policy.Dislike, Memory = Policy.Forever}, 
		{"High", Peasant = Policy.Hate, Memory = Policy.Forever}
	}
}

Policy.Load {
	Name = "Weregeld",
	Desc = "",
	Category = Policy.Law,
	Options = {"None", "Low", "Medium", "High"}
}

Policy.Load {
	Name = "Judge Authority",
	Desc = "",
	Category = Policy.Law,
	Options = {"Advice only", "Double vote", "Makes descision"}
}

Policy.Load {
	Name = "Marshall Authority",
	Desc = "Determines the amount of control the marshall has over the army.",
	Category = Policy.Law,
	Options = {
		{Name = "Raising Army", 
			{"Anyone", Warrior = Policy.Like, Memory = Policy.Medium},
			{"Permission requested", Warrior = Policy.Dislike, Memory = Policy.Medium},
			{"Marshall only", Warrior = Policy.Dislike, Memory = Policy.Long},
	}
}
