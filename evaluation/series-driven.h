#ifndef __SERIESDRIVEN__
#define __SERIESDRIVEN__

#include "evaluate.h"
#include "overlap-driven.h"


/* The data contained in every node of the prefix tree or DFA */
class series_data: public overlap_data {
  public:
	double_list occs;
    double_list::iterator occ_merge_point;
};


class series_driven: public overlap_driven{
  virtual void score_right(apta_node* right, int depth);
  virtual void score_left(apta_node* left, int depth);

protected:
  static DerivedRegister<series_driven> reg;

public:
  vector< state_set > left_dist;
  vector< state_set > right_dist;

  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void initialize(state_merger *);
  virtual void reset(state_merger *merger);
};

#endif
