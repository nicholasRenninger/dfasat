#ifndef __OVERLAPDRIVEN__
#define __OVERLAPDRIVEN__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class overlap_data: public evaluation_function {
  public:
    /* counts of positive and negative transition uses */
    num_map num_pos;
    num_map num_neg;

    /* counts of positive and negative endings */
    int num_accepting;
    int num_rejecting;
};

class overlap_driven: public evaluation_function{

protected:
  static DerivedRegister<overlap_driven> reg;

public:
  int overlap;
  
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
