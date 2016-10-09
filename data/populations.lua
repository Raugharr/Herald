Human = {
	Eats = {"Wheat", "Rye"},
	Drinks = {"Barley"}
}

Populations = {
	{
		Name = "Sheep",
		Nutrition = 9,
		MatureAge = {1, 7},
		DeathAge = 12,
		MaleRatio = 0.2,
		Eats = {"Hay"},
		Milk = 68, --Fluid ounces.
		Meat = 45,
		Hair = {
			Type = "Wool",
			Pounds = 6.5,
			IsShearable = true
		},
		Skin = {
			Type = "Leather",
			Pounds = 1
		},
		FMRatio = 20,
		Reproduce = {0.2, 1.8},
		SpaceReq = 5,
		Wealth = 0.1
	},
	
	{
		Name = "Ox",
		Nutrition = 45,
		MatureAge = {1, 15},
		DeathAge = 20,
		MaleRatio = 0.2,
		Eats = {"Hay"},
		Milk = 120,
		Meat = 240,
		Skin = {
			Type = "Leather",
			Pounds = 36
		},
		FMRatio = 20,
		Reproduce = {0.0, 1.0},
		SpaceReq = 5,
		Wealth = 1.0
	},
	
	{
		Name = "Pig",
		Nutrition = 9,
		MatureAge = {1, 4},
		DeathAge = 16,
		MaleRatio = 0.25,
		Eats = {"Barley", "Oats"},
		Milk = 10,
		Meat = 80,
		FMRatio = 10,
		Reproduce = {0.2, 4.0},
		SpaceReq = 5,
		Wealth = .2
	},
	
	{
		Name = "Chicken",
		Nutrition = 1,
		MatureAge = {1, 3},
		DeathAge = 5,
		MaleRatio = 0.15,
		Eats = {"Barley", "Oats"},
		Milk = 0,
		Meat = 3,
		FMRatio = 24,
		Reproduce = {0.8, 4.0},
		SpaceReq = 5,
		Wealth = 0.01
	},
	
	{
		Name = "Goat",
		Nutrition = 5,
		MatureAge = {1, 6},
		DeathAge = 10,
		MaleRatio = 0.2,
		Eats = {"Barley", "Oats", "Hay"},
		Milk = 12,
		Meat = 18,
		FMRatio = 20,
		Reproduce = {0.2, 1.8},
		SpaceReq = 5,
		Wealth = .2
	}
}
