#ifndef __AIKE__
#define __AIKE__

#include "evaluate.h"
#include "likelihood.h"

class aic: public likelihoodratio{

protected:
  REGISTER_DEC_TYPE(aic);

public:
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
};

#endif
