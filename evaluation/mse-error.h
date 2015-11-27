#ifndef __MSEERROR__
#define __MSEERROR__

#include "evaluate.h"

class mse_error: public evaluation_function{

protected:
  REGISTER_DEC_TYPE(mse_error);

public:
  double merge_error;

  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
