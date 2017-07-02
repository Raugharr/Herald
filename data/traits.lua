Traits = {
	{
		Name = "Brave",
		Likes = {"Loyal"},
		Dislikes = {"Coward"},
		Prevents = {"Coward"},
		Chance = 5
	},
	{
		Name = "Coward",
		Likes = {"Coward"},
		Dislikes = {"Brave", "Loyal"},
		Prevents = {"Brave"},
		Chance = 5
	},
	{
		Name = "Greedy",
		Likes = {"Ambitious"},
		Dislikes = {"Greedy"},
		Prevents = {},
		Chance = 5
	},
	{
		Name = "Loyal",
		Likes = {"Loyal", "Brave"},
		Dislikes = {"Coward"},
		Prevents = {},
		Chance = 5
	},
	{
		Name = "Ambitious",
		Likes = {"Greedy"},
		Dislikes = {"Coward"},
		Prevents = {},
		Chance = 5
	},
	{
		Name = "Glutton",
		Likes = {"Glutton"},
		Dislikes = {},
		Prevents = {},
		Chance = 5
	},
	{
		Name = "Honest",
		Likes = {"Honest", "Kind"},
		Dislikes = {"Greedy"},
		Prevents = {},
		Chance = 5
	},
	{
		Name = "Lazy",
		Likes = {},
		Dislikes = {"Ambitious"},
		Prevents = {"Ambitious"},
		Chance = 5
	},
	{
		Name = "Kind",
		Likes = {"Kind", "Honest"},
		Dislikes = {"Cruel"},
		Prevents = {"Cruel"},
		Chance = 5
	},
	{
		Name = "Cruel",
		Likes = {"Cruel", "Greedy"},
		Dislikes = {"Kind"},
		Prevents = {},
		Chance = 5
	},
	{
		Name = "Narcissist",
		Likes = {},
		Dislikes = {"Honest", "Narcissist"},
		Prevents = {},
		Chance = 2
	},
	{
		Name = "Alcholic",
		Likes = {"Alcholic"},
		Dislikes = {},
		Prevents = {},
		Chance = 2
	}
}
