#ifndef __DEPTHDRIVEN__
#define __DEPTHDRIVEN__

#include "evaluate.h"
#include "num_count.h"

class depth_data: public evaluation_data {

protected:
  REGISTER_DEC_DATATYPE(depth_data);

public:
    int depth;

    depth_data::depth_data();

    void read(int type, int index, int length, int symbol, string data);
};


class depth_driven: public evaluation_function{

protected:
  REGISTER_DEC_TYPE(depth_driven);

  double merge_error;
  
public:
  int  compute_score(state_merger*, apta_node* left, apta_node* right);
  void reset(state_merger *merger);
};

#endif
