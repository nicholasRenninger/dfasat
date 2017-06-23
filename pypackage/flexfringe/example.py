import sys, os, pprint
from flexfringe.estimators import DFASATEstimator, BaggingClassifier

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit("Usage: " + __file__ + " train_file")

    train_file = sys.argv[1]

    file_prefix = os.path.splitext(os.path.basename(train_file))[0]

    with open(train_file) as f:
        train = f.read()

    train_data = train.strip().split("\n")
    train_data_x = []
    train_data_y = []
    for line in train_data[1:]:
        train_data_x.append([str(i) for i in line.split(" ")])
        train_data_y.append(1)

    pprint.pprint(train_data_x[:5])
    pprint.pprint(train_data_y[:5])

    bag = BaggingClassifier(estimator=DFASATEstimator, number=50, output_file=file_prefix+'5.5.05', random_seed=True, random_counts=None,#[5, 15, 25],
#                      hData='kl_data', hName='kldistance',
                      hData='alergia_data', hName='alergia',
#                      hData='overlap_data', hName='overlap_driven',
#                      hData='likelihood_data', hName='likelihoodratio',
                      symbol_count=5, # -y
                      state_count=5, # -t
#                      parameter=0.5, # -p
#                      extend=0, # -x
                      apta_bound=1, # -b
                      dfa_bound=2000, # -d
                      sinkson=0, # -i
                      method=1, # -m
                      tries=1 # -n
                      )


#    est = sklearn.ensemble.BaggingClassifier(base_est, 2)
    fit = bag.fit(train_data_x, train_data_y, subset=True)
