function AI.Init()
	local Peasant = Behavior.Sequence {
					Behavior.Sequence {
							Behavior.Node("HasField"),
							Behavior.Selector {
									Behavior.Node("HasPlow"),
									Behavior.Node("MakeGood", "Plough", 1),
									},
							Behavior.Selector {
									Behavior.Node("HasReap"),
									Behavior.Node("MakeGood", "Sickle", 1),
									},
							Behavior.Node("WorkFields"),
							},
					Behavior.Selector {
							Behavior.Node("HasHouse"),
							Behavior.Node("ConstructBuilding"),
							},
					Behavior.Selector {
							Behavior.Node("HasAnimals"),
							Behavior.Selector {
										Behavior.Node("HasShelter"),
										Behavior.Node("ConstructBuilding"),
										},
							Behavior.Node("FeedAnimals"),
							},
					}
	local Woman = Behavior.Sequence {
					Behavior.Node("MakeFood")
					}
	local Child = Behavior.Node("Nothing")
	return {
		{"Peasant", Peasant},
		{"Woman", Woman},
		{"Child", Child}
	}
end

AI.SetAI = function(Person)
	return "Child"
end
