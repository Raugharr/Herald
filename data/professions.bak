Professions = {
	{
		Name = "Miller",
		Payment = "InKind",
		Goods = {"Flour"},
		InKind = {"Wheat", "Barley", "Oats"},
		Markup = 0.15,
		SecondaryJob = "Farmer",
		Behavior = Behavior.Node("Nothing")
	},
	{
		Name = "Metalsmith",
		Payment = "NoService",
		Goods = {"Spear", "Sword"},
		InKind = {"Iron Ore", "Animals"},
		Markup = 0.2,
		Behavior = Behavior.Node("Nothing")
	},
	{
		Name = "Bowyer",
		Payment = "NoService",
		Goods = {"Arrow", "Hunting Bow"},
		InKind = {"Food"},
		Markup = 0.15,
		SecondaryJob = "Farmer",
		Behavior = Behavior.Node("Nothing")
	},
	{
		Name = "Farmer",
		InKind = {"Flour"},
		Markup = 0.1,
		Goods = {},
		Behavior = Behavior.Sequence {
			Behavior.Selector {
				Behavior.Sequence {
					Behavior.Selector {
						Behavior.Node("HasAnimal", "Ox"),
						Behavior.Node("BuyAnimal", "Ox", 1)	
					},
					Behavior.Selector {
						Behavior.Node("HasGood", "Plough"),
						Behavior.Node("BuyGood", "Plough", 1)
					},
				},
				Behavior.Sequence {
					Behavior.Inverter("HasGood", "Hoe"),
					Behavior.Node("MakeGood", "Hoe", 1)
				}
			},
			Behavior.Sequence {
				Behavior.Inverter("HasGood", "Sickle"),
				Behavior.Node("MakeGood", "Sickle", 1)
			},
			Behavior.Sequence {
				Behavior.Inverter("Season", "Winter"),
				Behavior.Node("WorkFields")
			}
		},
		SeasonalWork = {
			October = {
				Behavior.Node("SlaughterAnimals")
			}
		}
	},
	{
		Name = "Herder",
		InKind = {"Wheat", "Rye", "Barley"},
		Goods = {},
		Markup = 0.1,
		Behavior = Behavior.Node("Nothing"),
		SeasonalWork = {
			October = {
				Behavior.Node("SlaughterAnimals")
			}
		}
	},
	{
		Name = "Hunter",
		InKind = {"What", "Rye", "Barley"},
		Goods = {},
		Markup = 0.1,
		Behavior = Behavior.Node("Hunt"),
		SecondaryJob = "Farmer"
	}
}