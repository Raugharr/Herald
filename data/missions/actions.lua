Mission.Load {
	Name = "Raise Popularity.",
	Description = "Attempt to increase how popular you are with everone.",
	OnTrigger = function(Frame)
		Frame.Owner:ChangePopularity(1)
	end,
	Action = Action.RaisePop,
	MeanTime = 90,
	Id = "Actions.1"
}
