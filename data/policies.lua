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
	Name = "Fiefdoms",
	Desc = "How much power a local lord has over his subjects weather they are completly free or peasants.",
	Category = Policy.Economy,
	Options = {
		{
			Name = "Freemen",
			Desc = "Every man is free and is not subject to the whims of any lord",
			Action = {
				SlaveRestriction = 0				
			}

		},
		{
			Name = "Slavery for outsiders",
			Desc = "Men are allowed to be forced into slavery but only if they are of a different culture.",
			Action = {
				SlaveRestriction = 1
			}
		},
		{
			Name = "Slavery permitted for all",
			Desc = "Outsiders and kinsmen are allowed to become slaves.",
			Action = {
				SlaveRestriction = 2
			}
		},
		{
			Name = "Peasant",
			Desc = "People under the lord are mearly peasants who rent their land from the lord and in return pay him taxes.",
			Action = {
				SlaveRestriction = 3
			}
		}
	}
}

World.LoadPolicy {
	Name = "Slavery length",
	Desc = "Determines how many generations of a family are permitted to be slaves.",
	Category = Policy.Economy,
	Options = {
		{
			Name = "Eternally slaves",
			Desc = "The children of a slave will become slaves as well.",
			Action = {
				SlaveLength = 0
			}
		},
		{
			Name = "Limited slavery",
			Desc = "The children of a slave will become freedmen instead of slaves and will have limited rights.",
			Action = {
				SlaveLength = 1
			}
		},
		{
			Name = "Rights of children",
			Desc = "The children of a slave will become freemen who will have all rights as any other man.",
			Action = {
				SlaveLength = 2
			}
		}
	}
}

World.LoadPolicy {
	Name = "Slavery reason",
	Desc = "Dictates the reasons a person can become a slave.",
	Category = Policy.Economy,
	Options = {
		{
			Name = "Willingly",
			Desc = "Sometimes it is better to have someone take care of you as a slave than to die as a freeman.",
			Action = {
				SlaveReason = 0
			}
		},
		{
			Name = "Raiding",
			Desc = "Captives that are taken from raids are allowed to be forced into slavery.",
			Action = {
				SlaveReason = 1
			}
		}
	}
}

World.LoadPolicy {
	Name = "Slavery Rights",
	Desc = "What rights a slave has.",
	Category = Policy.Economy,
	Options = {
		{
			Name = "None",
			Desc = "A slave is viewed as property and has no rights nor needs to be treated properly or cannot own property.",
			Action = {
				SlaveProperty = 0,
				SlaveRights = 0,
				SlaveTreatment = 0
			}
		},
		{
			Name = "Marginal",
			Desc = "While slaves still are viewed as property they must be given basic necessities that any freeman would expect.",
			Action = {
				SlaveProperty = 0,
				SlaveRights = 1,
				SlaveTreatment = 1
			}
		},
		{
			Name = "Allowed Property",
			Desc = "Slaves are allowed to own property.",
			Action = {
				SlaveProperty = 1,
				SlaveRights = 1,
				SlaveTreatment = 2,
			}
		}
	}
}

--[[World.LoadPolicy {
	Name = "Trade",
	Description = "The direction we wish to steer our merchants towards.",
	Category = Policy.Economy,

}--]]
