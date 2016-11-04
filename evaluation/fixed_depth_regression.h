#ifndef __FIXEDDEPTHREGRESSION__
#define __FIXEDDEPTHREGRESSION__

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
  
  void score_left(apta_node* left, int depth);
  void score_right(apta_node* right, int depth);
  void undo_score_left(apta_node* left, int depth);
  void undo_score_right(apta_node* right, int depth);
    
public:
  vector< state_set > l_dist;
  vector< state_set > r_dist;
  
  alergia ale;

  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void print_dot(FILE*, state_merger *);
  virtual int sink_type(apta_node* node);
  virtual bool sink_consistent(apta_node* node, int type);
  virtual int num_sink_types();
};

#endif
