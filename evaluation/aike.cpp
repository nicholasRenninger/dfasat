#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "state_merger.h"
#include "evaluate.h"
#include "depth-driven.h"
#include "aike.h"
#include "parameters.h"

REGISTER_DEF_DATATYPE(aic_data);
REGISTER_DEF_TYPE(aic);

/* Akaike Information Criterion (AIC), computes the AIC value and uses it as score, AIC increases are inconsistent */
bool aic::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return false;
  if (extra_parameters == 0) return false;

  double val = -2.0 * ( extra_parameters - (loglikelihood_orig - loglikelihood_merged) );

  if(val <= 0) return false;

  return true;
};

int aic::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  double val = -2.0 * ( extra_parameters - (loglikelihood_orig - loglikelihood_merged) );

  return (int)val * 100.0;
};
/* COMMENTED OUT, might be useful when computing distance from APTA distribution
 * as I understand it, the MDI algorithm computes it incremental from previous dist*/
/*
  init_perplexity = 0.0;
  init_parameters = 0;
  state_set states = merger->aut->get_states();
  for(state_set::iterator it = states.begin(); it != states.end(); ++it){
    double divider, count;
    divider = (double)(*it)->accepting_paths;
    for(int a = 0; a < alphabet_size; ++a){
      count = (double)(*it)->num_pos[a];
      if(count > 0.0){
        init_perplexity += count * (count / divider) * log(count / divider);
        init_parameters += 1;
      }
    }
  }
*/
