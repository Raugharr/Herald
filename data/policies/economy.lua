World.LoadPolicy {
	Name = "Fiefdoms",
	Desc = "How much power a local lord has over his subjects weather they are completly free or peasants.",
	Category = Policy.Economy,
	Options = {
		{
			Name = "Freemen",
			Desc = "Every man is free and is not subject to the whims of any lord",
			OnPass = {
				
			}

		},
		{
			Name = "Slavery for outsiders",
			Desc = "Men are allowed to be forced into slavery but only if they are of a different culture."
		},
		{
			Name = "Slavery permitted for all",
			Desc = "Outsiders and kinsmen are allowed to become slaves."
		},
		{
			Name = "Peasant",
			Desc = "People under the lord are mearly peasants who rent their land from the lord and in return pay him taxes."
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
			OnPass = {
				SlaveLength = 0
			}
		},
		{
			Name = "Limited slavery",
			Desc = "The children of a slave will become freedmen instead of slaves and will have limited rights.",
			OnPass = {
				SlaveLength = 1
			}
		},
		{
			Name = "Rights of children",
			Desc = "The children of a slave will become freemen who will have all rights as any other man.",
			OnPass = {
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
			OnPass = {
				SlaveReason = 0
			}
		},
		{
			Name = "Raiding",
			Desc = "Captives that are taken from raids are allowed to be forced into slavery.",
			OnPass = {
				SlaveReason = 1
			}
		},
	}
}

World.LoadPolicy {
	Name = "Slavery Rights",
	Desc = "What rights a slave has.",
	Category Policy.Economy,
	Options = {
		{
			Name = "None",
			Desc = "A slave is viewed as property and has no rights nor needs to be treated properly or cannot own property.",
			OnPass = {
				SlaveProperty = 0,
				SlaveRights = 0,
				SlaveTreatment = 0
			}
		},
		{
			Name = "Marginal",
			Desc = "While slaves still are viewed as property they must be given basic necessities that any freeman would expect."
			OnPass = {
				SlaveProperty = 0,
				SlaveRights = 1
				SlaveTreatment = 1
			}
		},
		{
			Name = "Allowed Property",
			Desc = "Slaves are allowed to own property."
			OnPass = {
				SlaveProperty = 1,
				SlaveRights = 1
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
