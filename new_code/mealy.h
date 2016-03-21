#ifndef __MEALY__
#define __MEALY__

#include "overlap-driven.h"

class mealy: public overlap_driven{

protected:
  REGISTER_DEC_TYPE(depth_driven);

public:
  double merge_error;

  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
