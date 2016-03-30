#ifndef __KLDISTANCE__
#define __KLDISTANCE__

#include "evaluate.h"
#include "depth-driven.h"

/* The data contained in every node of the prefix tree or DFA */
class likelihood_data: public alergia_data {
protected:
  REGISTER_DEC_TYPE(kldistance);
};

class kldistance: public alergia {

protected:
  REGISTER_DEC_TYPE(kldistance);

  void update_perplexity(double,double,double,double);

public:
  double perplexity;
  int extra_parameters;

  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif
