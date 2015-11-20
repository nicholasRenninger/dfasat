#ifndef __EVIDENCE__
#define __EVIDENCE__

#include "evaluate.h"

class evidence_driven: public evaluation_function {

protected:
  REGISTER_DEC_TYPE(evidence_driven);

public:
  int num_pos;
  int num_neg;

  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif
