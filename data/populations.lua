Human = {
	Eats = {"Wheat", "Rye"},
	Drinks = {"Barley"}
}

Populations = {
	{
		Name = "Sheep",
		Nutrition = 9,
		MatureAge = {ToYears(1), ToYears(7)},
		DeathAge = ToYears(12),
		MaleRatio = 0.2,
		Eats = {"Hay", "Straw"},
		Milk = 68, --Fluid ounces.
		Meat = 45
	},
	
	{
		Name = "Ox",
		Nutrition = 45,
		MatureAge = {ToYears(1), ToYears(15)},
		DeathAge = ToYears(20),
		MaleRatio = 0.2,
		Eats = {"Hay", "Straw"},
		Milk = 120,
		Meat = 1600
	},
	
	{
		Name = "Pig",
		Nutrition = 9,
		MatureAge = {ToYears(1), ToYears(4)},
		DeathAge = ToYears(16),
		MaleRatio = 0.25,
		Eats = {"Barley", "Oats"},
		Milk = 10,
		Meat = 45
	},
	
	{
		Name = "Chicken",
		Nutrition = 1,
		MatureAge = {ToYears(1), ToYears(3)},
		DeathAge = ToYears(5),
		MaleRatio = 0.05,
		Eats = {"Barley", "Oats"},
		Milk = 0,
		Meat = 3
	},
	
	{
		Name = "Goat",
		Nutrition = 5,
		MatureAge = {ToYears(1), ToYears(6)},
		DeathAge = ToYears(10),
		MaleRatio = 0.2,
		Eats = {"Barley", "Oats", "Hay", "Straw"},
		Milk = 12,
		Meat = 18,
	}
}