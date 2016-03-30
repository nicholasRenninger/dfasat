#ifndef __OVERLAPDRIVEN__
#define __OVERLAPDRIVEN__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class overlap_data: public count_data{
protected:
  REGISTER_DEC_TYPE(overlap-driven);
};

class overlap_driven: public count_driven{

protected:
  REGISTER_DEC_TYPE(overlap-driven);

public:
  int overlap;
  
  bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  void update_score(state_merger *merger, apta_node* left, apta_node* right);
  int  compute_score(state_merger*, apta_node* left, apta_node* right);
  void reset(state_merger *merger);
};

#endif
