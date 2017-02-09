#ifndef __COUNT__
#define __COUNT__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class count_data: public evaluation_data {

protected:
  REGISTER_DEC_DATATYPE(count_data);

public:
    int num_accepting;
    int num_rejecting;
    int accepting_paths;
    int rejecting_paths;

    count_data();
    
    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void read_to(int type, int index, int length, int symbol, string data);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};

class count_driven: public evaluation_function {

protected:
  REGISTER_DEC_TYPE(count_driven);

public:
  int num_merges;

  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);

  virtual int sink_type(apta_node* node);
  virtual bool sink_consistent(apta_node* node, int type);
  virtual int num_sink_types();

  //virtual void print_dot(iostream&, state_merger *);
};

#endif
