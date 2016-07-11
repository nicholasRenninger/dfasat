#ifndef __DEPTHDRIVEN__
#define __DEPTHDRIVEN__

#include "mse-error.h"
#include "alergia.h"

/* The data contained in every node of the prefix tree or DFA */
class fixed_depth_mse_data: public mse_data {

protected:
  REGISTER_DEC_DATATYPE(fixed_depth_mse_data);

public:
    alergia_data ald;
    
    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void read_to(int type, int index, int length, int symbol, string data);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};

class fixed_depth_mse_error: public mse_error {

protected:
  REGISTER_DEC_TYPE(fixed_depth_mse_error);
    
public:
  vector< fixed_depth_mse_data > l_dist;
  vector< fixed_depth_mse_data > r_dist;

  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void print_dot(FILE*, state_merger *);
};

#endif
