ManorConstraints = {Min = 50, Max = 800, Interval = 50}
BabyAvg = {0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999}
AgeGroups = {5, 0, 2190, 4745, 5659, 21900, 36500}
FamilyTypes = {{0.8, "Farmer"}, {0.2, "Herder"}}

function CopyTable(Old)
	local New = {}
	for k,v in pairs(Old) do
		New[k] = v
	end
	return New
end

function Farmer(Size)
	local Table = {}
	local Wheat = Crop("Wheat")
	local Rye = Crop("Rye")
	local Barley = Crop("Barley")
	local Oats = Crop("Oats")
	
	Table.Goods = {
		{"Wheat", Wheat.PerAcre * 15 * Size + (Wheat.PerAcre * 15)},
		{"Rye", Rye.PerAcre * 5},
		{"Barley", Barley.PerAcre * 5},
		{"Oats", Oats.PerAcre * 5},
		{"Scratch Plow", 1}
	}
	Table.Buildings = {{Size = 25, Floor = "Dirt", Walls = "Planks", Roof = "Hay"}}
	Table.Animals = {}
	return Table
end

function Herder(Size)
	local Table = {}
	local Goat = Animal("Goat")
	local Pig = Animal("Pig")
	local GoatCt = Size
	local PigCt = Size * 2
	
	Table.Goods = {{"Barley", (Pig.Nutrition * 356 * GoatCt) + (Goat.Nutrition * 365 * GoatCt)}}
	Table.Buildings = {{Size = 30, Floor = "Dirt", Walls = "Planks", Roof = "Hay"}}
	Table.Animals = {{"Goat", GoatCt}, {"Pig", GoatCt}}
	return Table
end