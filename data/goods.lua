Goods = {
	{Name = "Flour", Category = GoodCat.Ingredient, InputGoods = {{"Wheat", 1.4285714}}, Produce = {Time = 10}},
	{Name = "Leather", Category = GoodCat.Material, InputGoods = {}, Produce = {Time = 10}},
	{Name = "Meat", Category = GoodCat.Food, InputGoods = {}, Nutrition = 3, Produce = {Time = 10}},
	{Name = "Straw", Category = GoodCat.Material, InputGoods = {}, Produce = {Time = 10}},
	{Name = "Wool", Category = GoodCat.Other, InputGoods = {}, Produce = {Time = 10}},
	{Name = "Dirt", Category = GoodCat.Material, InputGoods = {}, Produce = {Time = 10}},

	{Name = "Wood", Category = GoodCat.Material, InputGoods = {}, Produce = {Time = 1}},
	{Name = "Iron Ore", Category = GoodCat.Material, InputGoods = {}, Produce = {Time = 1}},
	--{Name = "Lumber", Category = GoodCat.Material, InputGoods = {{"Wood", 5}}},
	{Name = "Board", Category = GoodCat.Material, InputGoods = {{"Wood", 16}}, Produce = {Time = 10}},
	{Name = "Wood Staff", Category = GoodCat.Material, InputGoods = {{"Board", 5}}, Produce = {Time = 20}},
	{Name = "Handle", Category = GoodCat.Material, InputGoods = {{"Board", 1}}, Produce = {Time = 15}},
	{Name = "Wood Craft", Category = GoodCat.Other, InputGoods = {{"Wood", 16}}, Produce = {Time = 50}},
	{Name = "Wood Tool", Category = GoodCat.Other, InputGoods = {{"Wood", 16 * 5}}, Produce = {Time = 50}},
	{Name = "Iron Tool", Category = GoodCat.Other, InputGoods = {{"Iron", 16}}, Produce = {Time = 100}},

	{
		Name = "Charcoal",
		Category = GoodCat.Material,
		InputGoods = {{"Wood", 1}},
		Produce ={
			--Type = "Batch", 
			--MaxBatch = 800, 
			--Time = 200
			Time = 1
		}
	},
	{
		Name = "Iron", 
		Category = GoodCat.Material,
		InputGoods = {
			{"Charcoal", 2},
			{"Iron Ore", 1}
		},
		Produce = {
			Type = "Single",
			Time = 10
		}
	},

	{
		Name = "Seax",
		Category = GoodCat.Weapon,
		Type = "Sword",
		MeleeAttack = 20,
		InputGoods = {
			{"Charcoal",28}, 
			{"Iron", 28},
			{"Handle", 1}
		},
		Produce = {Type = "Single", Time = 700}
	},
	{
		Name = "Spear",
		Category = GoodCat.Weapon,
		Type = "Spear",
		MeleeAttack = 30,
		InputGoods = {
			{"Charcoal", 16},
			{"Iron", 16},
			{"Wood Staff", 1}
		},
		Produce = {
			Type = "Single",
			Time = 700
		}
	},
	{
		Name = "Two-Handed Axe",
		Category = GoodCat.Weapon,
		Type = "Axe",
		MeleeAttack = 40,
		InputGoods = {
			{"Charcoal", 36},
			{"Iron", 36},
			{"Wood Staff", 1}
		},
		Produce = {
			Type = "Single",
			Time = 1400
		}
	},
	{
		Name = "Axe",
		Category = GoodCat.Weapon,
		Type = "Axe",
		MeleeAttack = 20,
		InputGoods = {
			{"Charcoal", 24},
			{"Iron", 24},
			{"Wood Staff", 1}
		},
		Produce = {
			Type = "Single",
			Time = 1400
		}
	},
	{
		Name = "Sword",
		Category = GoodCat.Weapon,
		Type = "Sword",
		MeleeAttack = 40,
		InputGoods = {
			{"Charcoal", 60},
			{"Iron", 60},
			{"Handle", 1}
		},
		Produce = {
			Type = "Single",
			Time = 2100
		}
	},
	{
		Name = "Shield",
		Category = GoodCat.Armor,
		Type = "Shield",
		Defense = 5,
		InputGoods = {},
		Produce = {
			Type = "Single",
			Time = 200
		}
	},
	{
		Name = "Javelin",
		Category = GoodCat.Weapon,
		Type = "Javelin",
		Damage = 2,
		RangeDamage = 3,
		Impact = 1,
		InputGoods = {
			--Why not do Charcoal = 12 instead.
			{"Wood", 48},
			{"Iron", 12}
		},
		Produce = {
			Time = 350
		}
	},
	{
		Name = "Leather Armor",
	 	Category = GoodCat.Armor,
		Type = "Armor",
		Defense = 5,
		Produce = {
			Type = "Single",
			Time = 500
		},
		InputGoods = {}
	},

	{Name = "Wool Yarn", Category = GoodCat.Material, Produce = {Type = "Single", Time = 1}, InputGoods = {{"Wool", 1}}},
	{Name = "Wool Clothing", Category = GoodCat.Clothing, Produce = {Type = "Single", Time = 500}, InputGoods = {{"Wool Yarn", 15--[[57600]]}}},

	{Name = "Tanned Hide", Category = GoodCat.Material, Product = {Time = 500}, Produce = {Type = "Single", Time = 1}, InputGoods = {{"Leather", 16}}},
	{Name = "Leather Good", Category = GoodCat.Material, Product = {Time = 20}, Produce = {Type = "Single", Time = 1}, InputGoods = {{"Tanned Hide", 1}}},
	{Name = "Quiver", Category = GoodCat.Material, Product = {Time = 20}, Produce = {Type = "Single", Time = 1}, InputGoods = {{"Leather", 16}}},

	{Name = "Bow", Category = GoodCat.Weapon, Type = "Bow", MeleeAttack = 0, RangeAttack = 15, Product = {Time = 2000}, Produce = {Type = "Single", Time = 1}, InputGoods = {{"Wood Staff", 1}}},
	{Name = "Arrow", Category = GoodCat.Material, RangeDamage = 1, Product = {Time = 20}, Produce = {Type = "Single", Time = 1}, InputGoods = {{"Iron", 1}, {"Wood", 4}}}

	--{Name = "Leather Body Armor", Category = GoodCat.Armor, Type = GoodCat.Armor, Defense = 10, InputGoods = {}},
	--{Name = "Full Leather Body Armor", Category = GoodCat.Armor, Type = GoodCat.Armor, Defense = 15, InputGoods = {{"Leather Body Armor", 1}, {"Partial Leather Armor", 1}}},
--	{Name = "Shear", Category = "Tool", Function = "Cut", Quality = 1, InputGoods = {{"Iron", 2}, {"Wood", 2}}},
--	{Name = "Scratch Plow", Category = "Tool", Function = "Plow", Quality = 1000, InputGoods = {{"Wood", 6}}},
--	{Name = "Hoe", Category = "Tool", Function = "Plow", Quality = 25, InputGoods = {{"Wood", 1}}},
--	{Name = "Stone", Category = GoodCat.Material, InputGoods = {}},
--	{Name = "Sickle", Category = "Tool", Function = "Reap", Quality = 500, InputGoods = {}},
--	{Name = "Stone Sickle", Category = "Tool", Function = "Reap", Quality = 400, InputGoods = {}},
--	{Name = "Wool Yarn", Category = GoodCat.Material, InputGoods = {{"Wool", 1}}},
	--{Name = "Wool Trousers", Category = GoodCat.Clothing, Locations = {"Ankles", "Lower Legs", "Upper Legs", "Pelvis"}, InputGoods = {{"Wool Yarn", 57600}}},
--	{Name = "Quern", Category = GoodCat.Material, InputGoods = {{"Stone", 3}}, OutputGoods = {{"Flour", 300}}},
}
