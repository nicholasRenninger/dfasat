#ifndef __LIKELIHOOD__
#define __LIKELIHOOD__

#include "evaluate.h"

class likelihoodratio: public evaluation_function {

protected:
  REGISTER_DEC_TYPE(likelihoodratio);

public:
  double loglikelihood_orig;
  double loglikelihood_merged;
  int extra_parameters;

  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif
