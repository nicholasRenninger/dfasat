#ifndef __CONFLICTDRIVENOVERLAP__
#define __CONFLICTDRIVENOVERLAP__

#include "overlap-driven.h"

/* The data contained in every node of the prefix tree or DFA */
class conflict_overlap_data: public overlap_data {

  REGISTER_DEC_DATATYPE(conflict_overlap_data);

  public:
	set<apta_node*> conflicts;
    set<apta_node*> undo_info;
    
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
};


class conflict_overlap_driven: public overlap_driven {
protected:
  REGISTER_DEC_TYPE(conflict_overlap_driven);

  int score;

public:
  virtual void update_score(state_merger*, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual void initialize(state_merger *);
};

#endif
