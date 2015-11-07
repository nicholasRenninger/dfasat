#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

/* Overlap driven, count overlap in positive transitions, used in Stamina winner */
bool overlap_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::consistent(merger,left,right) == false) return false;
    if(left->accepting_paths >= STATE_COUNT){
    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & left->pos((*it).first) == 0){
        inconsistency_found = true;
        return false;        
      }
    }
    for(num_map::iterator it = right->num_neg.begin();it != right->num_neg.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & left->neg((*it).first) == 0){
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
    for(num_map::iterator it = left->num_neg.begin();it != left->num_neg.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & right->neg((*it).first) == 0){
        inconsistency_found = true;
        return false;        
      }
    }
  }
  return true;
};

void overlap_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return;
  if (consistent(merger, left, right) == false) return;
  
  for(int i = 0; i < alphabet_size; ++i){
    if(left->pos(i) != 0 && right->pos(i) != 0){
      overlap += 1;
    }
    if(left->neg(i) != 0 && right->neg(i) != 0){
      overlap += 1;
    }
  }
};

bool overlap_driven::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;
    if(left->depth != right->depth){ inconsistency_found = true; return false; }
    return true;
};

int overlap_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) overlap = overlap * 2;
  return overlap;
};

void overlap_driven::reset(state_merger *merger){
  inconsistency_found = false;
  overlap = 0;
};


