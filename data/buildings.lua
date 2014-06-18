Cottage = {
		Name = "Cottage",
		Width = 25,
		Length = 20,
		Roof = "Hay",
		Floor = "None",
		Walls = "Stick",
		Output = {},
		Residents = "All"
}

BakerCottage = Cottage
BakerCottage.Name = "Baker Cottage"
BakerCottage.Output = {{"Bread Loaf", 4}}
BakerCottage.Residents = "Human"

MillerCottage = Cottage
MillerCottage.Name = "Miller Cottage"
MillerCottage.Output = {{"Flour", 128}}
MillerCottage.Residents = "Human"

Buildings = {
	MillerCottage,
	BakerCottage,
	Cottage
}
