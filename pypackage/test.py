from dfasat import DFASATEstimator

t = DFASATEstimator()

with open('train_toy.txt') as f:
  data = f.readlines()

t.fit_(dfa_data=data)





print("-----------------------")

t.fit_(dfa_data="""3 8
1 4 0/0 4/-0.0585381551032 0/0.0149159610922 3/-0.36993544381
1 4 4/-0.0585381551032 0/0.0149159610922 3/-0.36993544381 3/-0.0315366937024
1 4 0/0.0149159610922 3/-0.36993544381 4/-0.0315366937024 3/-0.332475438308""".split("\n"))


print("------------------------")

t2 = DFASATEstimator()
t2.fit_(dfa_file="train_toy.txt") 




