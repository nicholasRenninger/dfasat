
#ifndef _DEPTH_DRIVEN_H_
#define _DEPTH_DRIVEN_H_

#include "evaluate.h"

using namespace std;

class depth_driven: public evaluation_function{
public:
  int depth;
  
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};


#endif /* _EVALUATE_H_ */
