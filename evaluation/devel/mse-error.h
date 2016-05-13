#ifndef __MSEERROR__
#define __MSEERROR__

#include "evaluate.h"

typedef double_list = list<double>;

/* The data contained in every node of the prefix tree or DFA */
class mse_data: public evaluation_data {

protected:
  REGISTER_DEC_DATATYPE(mse_data);

public:
    /* occurences of this state */
    double_list occs;
    double mean;
    int merge_point;

    mse_data();

    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};

class mse_error: public evaluation_function{

protected:
  REGISTER_DEC_TYPE(mse_error);

public:
  int num_merges = 0;
  int num_points = 0;
  double RSS_before = 0.0;
  double RSS_after = 0.0;
  
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual void print_dot(FILE*, state_merger *);
};

#endif
