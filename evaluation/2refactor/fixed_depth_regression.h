#ifndef __DEPTHDRIVEN__
#define __DEPTHDRIVEN__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class fixed_depth_regression_data: public regression_data {
protected:
  REGISTER_DEC_TYPE(kldistance);
};

class fixed_depth_regression: public regression_function{


protected:
  REGISTER_DEC_TYPE(fixed_depth_mse);

  double merge_error;
  
public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
