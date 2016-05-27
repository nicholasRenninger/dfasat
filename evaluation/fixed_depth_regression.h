#ifndef __DEPTHDRIVEN__
#define __DEPTHDRIVEN__

#include "mse-error.h"

/* The data contained in every node of the prefix tree or DFA */
class fixed_depth_mse_data: public mse_data {

protected:
  REGISTER_DEC_DATATYPE(fixed_depth_mse_data);

public:
    int num_accepting;
    int num_rejecting;
    int accepting_paths;
    int rejecting_paths;
    /* counts of positive and negative transition uses */
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
    
    fixed_depth_mse_data();
    
    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void read_to(int type, int index, int length, int symbol, string data);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};

class fixed_depth_mse_error: public mse_error {

protected:
  REGISTER_DEC_TYPE(fixed_depth_mse_error);
    
public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void print_dot(FILE*, state_merger *);
};

#endif
