
#ifndef _DFASAT_H_
#define _DFASAT_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include "state_merger.h"

using namespace std;

int dfasat(state_merger &merger,
           const char* sat_program,
           const char* dot_output,
           const char* aut_output);

#endif /* _DFASAT_H_ */
