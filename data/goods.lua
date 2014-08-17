Goods = {
	{
		Name = "Iron",
		Category = "Material",
		InputGoods = {}
	},
	
	{
		Name = "Log",
		Category = "Material",
		InputGoods = {}
	},
	
	{
		Name = "Board",
		Category = "Material",
		InputGoods = {}
	},
	
	{
		Name = "Stone",
		Category = "Material",
		InputGoods = {}
	},
	
	{
		Name = "Wool",
		Category = "Other",
		InputGoods = {}
	},
	
	{
		Name = "Flour",
		Category = "Ingredient",
		InputGoods = {{"Wheat", 1}}
	},
	
	{
		Name = "Bread Loaf",
		Category = "Food",
		InputGoods = {{"Flour", 12}}
	},
	
	{
		Name = "Straw",
		Category = "Food",
		InputGoods = {},
		Nutrition = 3 --Per pound
	},
	
	{
		Name = "Hay",
		Category = "Food",
		InputGoods = {},
		Nutrition = 5 --Per pound
	},
	
	{
		Name = "Meat",
		Category = "Food",
		InputGoods = {},
		Nutrition = 3 --Per pound, exception is hard coded.
	},
	
	{
		Name = "Stone Sickle",
		Category = "Tool",
		Function = "Reap",
		InputGoods = {}
	},
	
	{
		Name = "Shear",
		Category = "Tool",
		Function = "Cut",
		InputGoods = {{"Iron", 2}, {"Wood", 2}} 
	},
	
	{
		Name = "Scratch Plow",
		Category = "Tool",
		Function = "Plow",
		InputGoods = {{"Wood", 6}} 
	},
	
	{
		Name = "Wool Trousers",
		Category = "Clothing",
		InputGoods = {{"Wool", 12}}
	},
	
	{
		Name = "Wool Tunic",
		Category = "Clothing",
		InputGoods = {{"Wool", 12}}
	}
}