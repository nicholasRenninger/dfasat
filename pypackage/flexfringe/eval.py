#!/usr/bin/python3

import re
from scipy.stats import rv_discrete
import sys
import numpy as np

# we should define these somewhere that is associated with the
# evaluation_functino. Maybe member variables that are initialized in
# the print_dot function ... but that would make evaluation depend on
# the output format, which is not very elegant.
# technically we should use the apta after merging, and
MEAN_REGEX = '(?P<state>\d+) \[shape=(doublecircle|circle|ellipse) label=\"\[(?P<mean>\d+)\].*\"\];'
SYMLST_REGEX = "((?P<sym>-?\d+) \[(?P<occ>\d+):\d+\])+"
TRAN_REGEX = "^(?P<sst>.+) -> (?P<dst>.+) \[label=\"(?P<slst>.+)\"[ style=dotted]*\];$"

def load_means(content):
    means = []

    for line in content.split("\n"):
      matcher = re.match(MEAN_REGEX, line.strip())
#      print(line.strip())
#      print(MEAN_REGEX)
      if matcher is None: continue
      #for match in matcher:
      match = matcher
      if match.group("mean") == "0": continue
      means.append("{} -> {} [label=\" {} [{}:0]\"];".format(match.group("state"), match.group("state"),
                                                              "-1", match.group("mean")))
    return means

def load_model_from_file(dot_path, normalize=False):
    with open(dot_path, "r") as f:
        return load_model(f.read(), normalize)

# Read and extract automaton structure from dot file
# returns: dictionary dict[currect_state][symbol_read] = next_state
def load_model(dot_string, normalize=False, with_means=False):
    model = {}
    if with_means:
      means = "\n".join(load_means(dot_string))
      dot_string = means + dot_string

    for line in dot_string.split("\n"):
        matcher = re.match(TRAN_REGEX, line.strip())
        if matcher is not None:
            sstate = matcher.group("sst")
            dstate = matcher.group("dst")
            symlist = matcher.group("slst")
            if sstate not in model:
                model[sstate] = {}
            for smatcher in re.finditer(SYMLST_REGEX, symlist):
                symbol = int(smatcher.group("sym"))
                occurrences = int(smatcher.group("occ"))
                if symbol not in model[sstate]:
                    model[sstate][symbol] = (dstate, occurrences)
                else:
                    gamma = 0

    # normalizing occurrence counts to probabilities
    if normalize:
        for state in model:
            ssum = sum(map(lambda x: x[1], model[state].values()))
            for symbol in model[state]:
                dstate, occurrences = model[state][symbol]
                model[state][symbol] = (dstate, occurrences / float(ssum))
    return model



# given model, make predictions for all lines in test-path and
# write result to pred-path
def get_word_acceptance(model, test_array):
    REJECT = 0
    ACCEPT = 1

    accept = 0
    waccept = 0
    reject = 0
    okwindows = 0
    wneg = 0
    pred_array = []

    for line in test_array:
        state = "0"

        symbol_sequence=[]

        #for actual_symbol in sequence:
        for i in range(2, len(line.strip().split(" "))):
            symbol_sequence.append(int(line.strip().split(" ")[i].split(",")[0]))

        #print symbol_sequence

        # loop over word
        rejected = False
        for i in range(len(symbol_sequence)):
            actual_symbol = symbol_sequence[i]

            if state not in model:
                pred_array.append(REJECT)
                rejected = True
                break
            else:
                try:
                    state = model[state][actual_symbol][0]
                except KeyError:
                    rejected = True
                    pred_array.append(REJECT)
                    #print "in state %s read %s, no transition" % (state, actual_symbol)
                    #print model[state]
                    reject+=1
                    break

        if not rejected:
            pred_array.append(ACCEPT)
            #print "Accepted %s" % (accept+reject)
            accept+=1
            waccept+=1

        else:
            gamma = 0
            #print "Rejected"

        if ((accept+reject) % 5) == 0:
           if waccept > 3:
#             pred_array.append("windowok")
             #print "Window ok"
             okwindows+=1
           else:
             if waccept == 0:
               wneg+=1
           waccept = 0

    if accept == 0:
        #print "no acceptance"
        accept = 1
    if reject == 0:
        #print "no reject"
        reject = 1

     # this output is for the CNSM/LCN papers
#     if float(accept)/(accept+reject) > 0.75:
#         print("BOTNET")
#     else:
#         print("CLEAN")

     # this return should probably an array of (0/1, prob) for the
     # tst input argument instead of putting it into the output file
    print("Acc %s Rej %s, total %s, ratio %s, quota %s, Windows: %s of %s, %s neg" % (accept, reject, float(accept)/(accept+reject), float(accept)/reject, float(reject)/accept, okwindows, (accept+reject)/5, wneg))

    return pred_array


def predict(prefix, model):
  # traverse
  state = "0"
  for s in prefix.split(" "):
    if s == "": continue
    try:
      state = model[state][int(s)][0]
#      print(state)
    except:
#      print("hello")
      break
  if len(model[state].items()) == 0:
    return [(-1, (0, 1))]
  return sorted(model[state].items(), key=lambda x: x[1][1], reverse=True)

if __name__ == "__main__":

    # you can also invoke the script from the command line
    if len(sys.argv) > 1:
        dot = sys.argv[1]
        tst = sys.argv[2]
        of = sys.argv[3]
    else:
        print("usesage: ./name dotfile testfile outputfile")

    m = load_model_from_file(dot, normalize=True)

    # print m
    get_word_acceptance(m, tst, of)


