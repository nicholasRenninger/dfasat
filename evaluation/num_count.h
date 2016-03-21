#ifndef __EVIDENCE__
#define __EVIDENCE__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class edsm_data: public evaluation_data {
  public:
  int num_accepting;
  int num_rejecting;
};

class evidence_driven: public evaluation_function {

protected:
  REGISTER_DEC_TYPE(evidence_driven);

public:
  int num_pos;
  int num_neg;

  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual void undo_update(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif
