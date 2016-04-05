#ifndef __EVIDENCE__
#define __EVIDENCE__

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
    
    void read(int type, int index, int length, int symbol, string data);
    void update(evaluation_data* right);
    void undo(evaluation_data* right);
};

class count_driven: public evaluation_function {

protected:
  REGISTER_DEC_TYPE(count_driven);

public:
  int num_merges;

  void update_score(state_merger *merger, apta_node* left, apta_node* right);
  void undo_update(state_merger *merger, apta_node* left, apta_node* right);
  int  compute_score(state_merger*, apta_node* left, apta_node* right);
  void reset(state_merger *merger);
};

#endif
