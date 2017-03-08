#ifndef __COUNT__
#define __COUNT__

#include "evaluate.h"

class mealy_data;

typedef map<int, int> output_map;
typedef map<int, mealy_data*> undo_map;
typedef map<int, string> is_map;
typedef map<string, int> si_map;

/* The data contained in every node of the prefix tree or DFA */
class mealy_data: public evaluation_data {

protected:
  REGISTER_DEC_DATATYPE(mealy_data);

public:
    output_map  outputs;
    undo_map    undo_info;
    
    static int num_outputs;
    static si_map output_int;
    static is_map int_output;
    
    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};

class mealy: public evaluation_function {

protected:
  REGISTER_DEC_TYPE(mealy);

public:

  int num_matched;
  int num_unmatched;

  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void print_dot(iostream&, state_merger *);
  virtual bool compute_consistency(state_merger *, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *);
};

#endif