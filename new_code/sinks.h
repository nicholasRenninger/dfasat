
#ifndef _SINKS_H_
#define _SINKS_H_

#include "state_merger.h"

using namespace std;

extern bool USE_SINKS;
extern bool MERGE_SINKS_DSOLVE;

/* Return a sink type, or -1 if no sink 
 * Sinks are special states that optionally are not considered as merge candidates, 
 * and are optionally merged into one (for every type) before starting exact solving */
int sink_type(apta_node* node);
bool sink_consistent(apta_node* node, int type);
int num_sink_types();

#endif /* _SINKS_H_ */