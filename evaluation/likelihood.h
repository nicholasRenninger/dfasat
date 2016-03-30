#ifndef __LIKELIHOOD__
#define __LIKELIHOOD__

#include "evaluate.h"

/* The data contained in every node of the prefix tree or DFA */
class likelihood_data: public alergia_data {
protected:
  REGISTER_DEC_TYPE(likelihoodratio);
};

class likelihoodratio: public alergia {

protected:
  REGISTER_DEC_TYPE(likelihoodratio);
  
  void update_likelihood(double,double,double,double);

public:
  double loglikelihood_orig;
  double loglikelihood_merged;
  int extra_parameters;

  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif
