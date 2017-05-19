#ifndef __CONFLICTDRIVEN__
#define __CONFLICTDRIVEN__

#include "num_count.h"

/* The data contained in every node of the prefix tree or DFA */
class conflict_data: public count_data {

  REGISTER_DEC_DATATYPE(conflict_data);


  public:
	set<apta_node*> conflicts;
    set<apta_node*> undo_info;
    
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};


class conflict_driven: public count_driven{
protected:
  REGISTER_DEC_TYPE(conflict_driven);

  int score;

public:
  virtual void update_score(state_merger*, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual void initialize(state_merger *);
};

#endif
