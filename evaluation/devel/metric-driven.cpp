#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "metric-driven.h"

DerivedRegister<metric_driven> metric_driven::reg("metric_driven");

/* Metric driven merging, addition by chrham */
bool metric_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(evaluation_function::consistent(merger,left,right) == false) return false;

  if(left->accepting_paths >= STATE_COUNT){
    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & left->pos((*it).first) == 0){
        inconsistency_found = true;
        return false;        
      }
    }
  }
  if(right->accepting_paths >= STATE_COUNT){
    for(num_map::iterator it = left->num_pos.begin();it != left->num_pos.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & right->pos((*it).first) == 0){
        inconsistency_found = true;
        return false;
      }
    }
  }
  return true;
};

// what's a good score?
void metric_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return;
  if (consistent(merger, left, right) == false) return;
  
  for(int i = 0; i < alphabet_size; ++i){
    if(left->pos(i) != 0 && right->pos(i) != 0){
      overlap += 1;
    }
  }
};

bool metric_driven::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;
    return true;
};

int metric_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return overlap;
};

void metric_driven::reset(state_merger *merger){
  inconsistency_found = false;
  overlap = 0;
};

/* end addition */


