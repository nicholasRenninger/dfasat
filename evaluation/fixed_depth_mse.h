#ifndef __DEPTHDRIVEN__
#define __DEPTHDRIVEN__

#include "evaluate.h"

class depth_driven: public evaluation_function{


protected:
  REGISTER_DEC_TYPE(depth_driven);

  double merge_error;
  
public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
