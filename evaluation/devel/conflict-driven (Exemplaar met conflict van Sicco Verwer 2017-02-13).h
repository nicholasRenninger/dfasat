#ifndef __OVERLAPDRIVEN__
#define __OVERLAPDRIVEN__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class conflict_data: public evaluation_data {
  public:
	node_list conflicts;
    node_list::iterator merge_point;
};


class conflict_driven: public evaluation_function{

protected:
  static DerivedRegister<overlap_driven> reg;

public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
