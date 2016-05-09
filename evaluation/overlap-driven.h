#ifndef __OVERLAPDRIVEN__
#define __OVERLAPDRIVEN__

#include "alergia.h"

/* The data contained in every node of the prefix tree or DFA */
class overlap_data: public alergia_data {
protected:
  REGISTER_DEC_DATATYPE(overlap_data);
};

class overlap_driven: public alergia {

protected:
  REGISTER_DEC_TYPE(overlap_driven);

public:
  int overlap;
  int non_overlap;
  
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif
