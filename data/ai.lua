function AI.Init()
	local Peasant = AI.CompNode("Sequence", {
					AI.CompNode("Sequence", {
							AI.PrimNode("HasField"),
							AI.CompNode("Selector", {
									AI.PrimNode("HasPlow"),
									AI.PrimNode("MakeGood"),
									}),
							AI.CompNode("Selector", {
									AI.PrimNode("HasReap"),
									AI.PrimNode("MakeGood"),
									}),
							AI.PrimNode("WorkField"),
							}),
					AI.CompNode("Selector", {
							AI.PrimNode("HasHouse"),
							AI.PrimNode("ConstructBuilding"),
							}),
					AI.CompNode("Selector", {
							AI.PrimNode("HasAnimals"),
							AI.CompNode("Selector", {
										AI.PrimNode("HasShelter"),
										AI.PrimNode("ConstructBuilding"),
										}),
							AI.PrimNode("FeedAnimals"),
							}),
					})
	local Woman = AI.CompNode("Sequence", {
					AI.PrimNode("MakeFood")
					})
	local Child = AI.PrimNode("Nothing")

	AI.Hook(Child, Hook("Age", ToMonth("Years", 13)), 
		{
			AI.PrimNode("IsMale"),
			AI.DecorateNode("Not", "IsMale")
		}, {Peasant, Woman})
	return {
		{"Peasant", Peasant},
		{"Woman", Woman},
		{"Child", Child}
	}
end

AI.SetAI = function(Person)
	return "Child"
end
