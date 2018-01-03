Castes = {
	Serf = {
		Jobs = {"Farmer", "Herder"},
		Behavior = Behavior.Node("Nothing"),
	},
	Peasant = {
	Jobs = {"Farmer", "Herder", "Miller"},
		Behavior = Behavior.Sequence {
			Behavior.Sequence {
				Behavior.Selector {
					Behavior.Node("HasGood", "Spear"),
					Behavior.Node("BuyGood", "Spear", 1)
				},
				Behavior.Selector {
					Behavior.Node("HasGood", "Wooden Shield"),
					Behavior.Node("BuyGood", "Wooden Shield", 1)
				}
			}
		},
		},
	Craftsman = {
		Jobs = {"Bowyer", "Metalsmith"},
		Behavior = Behavior.Selector {
		Behavior.Selector {
			Behavior.Node("HasGood", "Spear"),
			Behavior.Node("BuyGood", "Spear", 1)
		},
		Behavior.Selector {
			Behavior.Node("HasGood", "Wooden Shield"),
			Behavior.Node("BuyGood", "Wooden Shield", 1)
		},
		Behavior.Selector {
			Behavior.Node("HasGood", "Leather Armor"),
			Behavior.Node("BuyGood", "Leather Armor", 1)
		}
	}
	},
	Warrior = {
		Jobs = {"Hunter"},
		Behavior = Behavior.Selector {
		Behavior.Selector {
			Behavior.Sequence {
				Behavior.Inverter("HasGood", "Sword"),
				Behavior.Node("BuyGood", "Sword", 1)
			},
			Behavior.Selector {
				Behavior.Node("HasGood", "Spear"),
				Behavior.Node("MakeGood", "Spear", 1),
				Behavior.Node("BuyGood", "Spear", 1)
			}
		}, 
		Behavior.Sequence {
			Behavior.Sequence {
				Behavior.Inverter("HasGood", "Leather Armor"),
				Behavior.Node("BuyGood", "Leather Armor", 1)
			},
			Behavior.Selector {
				Behavior.Node("HasGood", "Wooden Shield"),
				Behavior.Node("HasGood", "Wooden Shield"),
				Behavior.Node("PurchaseGood", "Wooden Shield", 1),
				Behavior.Node("MakeGood", "Wood Shield", 1)
			}
		},
		Behavior.Sequence {
			Behavior.Selector {
				Behavior.Node("HasGood", "Hunting Bow"),
				Behavior.Node("BuyGood", "Hunting Bow", 1),
				Behavior.Node("MakeGood", "Hunting Bow", 1)
			},
			Behavior.Sequence {
				--SetVarible("Quantity", 30),
				--SetVariable("Good", Behavior.Node("GetGood", "Arrow")),
				--Behavior.Node("LessThan", "Var.Quantity", "Good"),
				--Behavior.Node("Minus" "Var.Quantity", "Good"),
				Behavior.Node("BuyGood", "Arrow", 1)
			}
		}
	}
	}
}
