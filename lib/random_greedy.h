
#ifndef _RANDOM_GREEDY_H_
#define _RANDOM_GREEDY_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include "state_merger.h"
#include "refinement.h"

using namespace std;

const int RANDOMG = 1;
const int NORMALG = 2;

refinement_list* random_greedy_bounded_run(state_merger* merger);

#endif /* _GENERATOR_H_ */
