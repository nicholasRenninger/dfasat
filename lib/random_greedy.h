
#ifndef _RANDOM_GREEDY_H_
#define _RANDOM_GREEDY_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include "state_merger.h"

using namespace std;

extern int alphabet_size;
extern int GREEDY_METHOD;
extern int APTA_BOUND;
extern int CLIQUE_BOUND;
extern bool EXTEND_ANY_RED;

const int RANDOMG = 1;
const int NORMALG = 2;

merge_list random_greedy_bounded_run(state_merger* merger);

#endif /* _GENERATOR_H_ */
