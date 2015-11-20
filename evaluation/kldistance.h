#ifndef __KLDISTANCE__
#define __KLDISTANCE__

#include "evaluate.h"
#include "depth-driven.h"

class kldistance: public depth_driven {

protected:
  REGISTER_DEC_TYPE(kldistance);

public:
  double perplexity;
  int extra_parameters;

  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif
