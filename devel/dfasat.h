
#ifndef _DFASAT_H_
#define _DFASAT_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include "state_merger.h"

extern bool MERGE_SINKS_PRESOLVE;
extern int OFFSET;
extern int EXTRA_STATES;
extern bool TARGET_REJECTING;
extern bool SYMMETRY_BREAKING;
extern bool FORCING;

using namespace std;

int dfasat(state_merger &merger,
           const char* sat_program,
           const char* dot_output,
           const char* aut_output);

#endif /* _DFASAT_H_ */
