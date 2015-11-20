#ifndef __ALERGIA__
#define __ALERGIA__

#include "evaluate.h"
#include "depth-driven.h"

class alergia: public depth_driven {

protected:
  REGISTER_DEC_TYPE(alergia);

public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
