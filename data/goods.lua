Goods = {
	{Name = "Flour", Category = "Ingredient", InputGoods = {{"Wheat", 1.4285714}}, Produce = {Time = 10}},
	{Name = "Leather", Category = "Material", InputGoods = {}, Produce = {Time = 10}},
	{Name = "Meat", Category = "Food", InputGoods = {}, Nutrition = 3, Produce = {Time = 10}},
	{Name = "Straw", Category = "Material", InputGoods = {}, Produce = {Time = 10}},
	{Name = "Wool", Category = "Other", InputGoods = {}, Produce = {Time = 10}},
	{Name = "Dirt", Category = "Material", InputGoods = {}, Produce = {Time = 10}},

	{Name = "Wood", Category = "Material", InputGoods = {}, Produce = {Time = 1}},
	{Name = "Iron Ore", Category = "Material", InputGoods = {}, Produce = {Time = 1}},
	--{Name = "Lumber", Category = "Material", InputGoods = {{"Wood", 5}}},
	{Name = "Board", Category = "Material", InputGoods = {{"Wood", 16}}, Produce = {Time = 10}},
	{Name = "Wood Staff", Category = "Material", InputGoods = {{"Board", 5}}, Produce = {Time = 15}},
	{Name = "Handle", Category = "Material", InputGoods = {{"Board", 1}}, Produce = {Time = 15}},
	{Name = "Wood Craft", Category = "Other", InputGoods = {{"Wood", 16}}, Produce = {Time = 15}},
	{Name = "Wood Tool", Category = "Other", InputGoods = {{"Wood", 16}}, Produce = {Time = 15}},
	{Name = "Iron Tool", Category = "Other", InputGoods = {{"Iron", 16}}, Produce = {Time = 15}},

	{
		Name = "Charcoal",
		Category = "Material",
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
		Category = "Material",
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
		Category = "Weapon",
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
		Category = "Weapon",
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
		Category = "Weapon",
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
		Category = "Weapon",
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
		Category = "Weapon",
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
		Category = "Armor",
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
		Category = "Weapon",
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
	 	Category = "Armor",
		Type = "Armor",
		Defense = 5,
		Produce = {
			Type = "Single",
			Time = 500
		},
		InputGoods = {}
	},

	{Name = "Bow", Category = "Weapon", Type = "Bow", MeleeAttack = 0, RangeAttack = 15, Product = {Time = 2000}, InputGoods = {{"Wood Staff", 1}}},
	{Name = "Arrow", Category = "Material", RangeDamage = 1, Product = {Time = 20}, InputGoods = {{"Iron", 1}, {"Wood", 4}}}
	--{Name = "Leather Body Armor", Category = "Armor", Type = "Armor", Defense = 10, InputGoods = {}},
	--{Name = "Full Leather Body Armor", Category = "Armor", Type = "Armor", Defense = 15, InputGoods = {{"Leather Body Armor", 1}, {"Partial Leather Armor", 1}}},
--	{Name = "Shear", Category = "Tool", Function = "Cut", Quality = 1, InputGoods = {{"Iron", 2}, {"Wood", 2}}},
--	{Name = "Scratch Plow", Category = "Tool", Function = "Plow", Quality = 1000, InputGoods = {{"Wood", 6}}},
--	{Name = "Hoe", Category = "Tool", Function = "Plow", Quality = 25, InputGoods = {{"Wood", 1}}},
--	{Name = "Stone", Category = "Material", InputGoods = {}},
--	{Name = "Sickle", Category = "Tool", Function = "Reap", Quality = 500, InputGoods = {}},
--	{Name = "Stone Sickle", Category = "Tool", Function = "Reap", Quality = 400, InputGoods = {}},
--	{Name = "Wool Yarn", Category = "Material", InputGoods = {{"Wool", 1}}},
	--{Name = "Wool Tunic", Category = "Clothing", Locations = {"Upper Arms", "Upper Chest", "Lower Chest"}, InputGoods = {{"Wool Yarn", 57600}}},
	--{Name = "Wool Trousers", Category = "Clothing", Locations = {"Ankles", "Lower Legs", "Upper Legs", "Pelvis"}, InputGoods = {{"Wool Yarn", 57600}}},
--	{Name = "Quern", Category = "Material", InputGoods = {{"Stone", 3}}, OutputGoods = {{"Flour", 300}}},
}
