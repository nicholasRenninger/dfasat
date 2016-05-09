#ifndef __DEPTHDRIVEN__
#define __DEPTHDRIVEN__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class fixed_depth_regression_data: public regression_data {
protected:
  REGISTER_DEC_DATATYPE(fixed_depth_regression_data);

    num_map num_pos;
    num_map num_neg;
    
    inline int pos(int i){
        num_map::iterator it = num_pos.find(i);
        if(it == num_pos.end()) return 0;
        return (*it).second;
    }

    inline int neg(int i){
        num_map::iterator it = num_neg.find(i);
        if(it == num_neg.end()) return 0;
        return (*it).second;
    }
    
    alergia_data();

    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};

class fixed_depth_regression: public regression_function{


protected:
  REGISTER_DEC_TYPE(fixed_depth_regression);

  double merge_error;
  
public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

#endif
