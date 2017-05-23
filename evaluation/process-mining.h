#ifndef __PROCESSMINING__
#define __PROCESSMINING__

#include "overlap-driven.h"

/* The data contained in every node of the prefix tree or DFA */
class process_data: public overlap_data {
protected:
  REGISTER_DEC_DATATYPE(process_data);
public:
    set<int> done_tasks;
    set<int> future_tasks;
    set<int> undo_info;
    set<int> undo_dinfo;

    virtual void print_state_label(iostream& output, apta* aptacontext);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);

    virtual void print_state_label(iostream& output, apta* aptacontext);
};

class process_mining: public overlap_driven {
protected:
  REGISTER_DEC_TYPE(process_mining);
public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void initialize(state_merger *);
};

#endif
