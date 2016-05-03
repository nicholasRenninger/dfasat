#ifndef __METRICDRIVEN__
#define __METRICDRIVEN__

#include "evaluate.h"

class metric_driven: public evaluation_function{

protected:
  static DerivedRegister<metric_driven> reg;

public:
  int overlap;
  
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
