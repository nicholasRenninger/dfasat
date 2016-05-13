#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "overlap-driven.h"
#include "parameters.h"

REGISTER_DEF_DATATYPE(overlap_data);
REGISTER_DEF_TYPE(overlap_driven);
//DerivedRegister<overlap_driven> overlap_driven::reg("overlap_driven");

/* Overlap driven, count overlap in positive transitions, used in Stamina winner */
bool overlap_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(count_driven::consistent(merger, left, right) == false){ inconsistency_found = true; return false; }
    
    overlap_data* l = (overlap_data*) left->data;
    overlap_data* r = (overlap_data*) right->data;

    if(l->accepting_paths >= STATE_COUNT){
        for(num_map::iterator it = r->num_pos.begin(); it != r->num_pos.end(); ++it){
            if((*it).second >= SYMBOL_COUNT && l->pos((*it).first) == 0){
                inconsistency_found = true;
                return false;
                non_overlap += 1;
            }
        }
        /*for(num_map::iterator it = r->num_neg.begin(); it != r->num_neg.end(); ++it){
            if((*it).second >= SYMBOL_COUNT & l->neg((*it).first) == 0){
                inconsistency_found = true;
                return false;
            }
        }*/
    }
  
    if(r->accepting_paths >= STATE_COUNT){
        for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
            if((*it).second >= SYMBOL_COUNT && r->pos((*it).first) == 0){
                inconsistency_found = true;
                return false;
                non_overlap += 1;
            }
        }
        /*for(num_map::iterator it = l->num_neg.begin();it != l->num_neg.end(); ++it){
            if((*it).second >= SYMBOL_COUNT & r->neg((*it).first) == 0){
                inconsistency_found = true;
                return false;
            }
        }*/
  }
  return true;
};

void overlap_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
    overlap_data* l = (overlap_data*) left->data;
    overlap_data* r = (overlap_data*) right->data;
    
    if (inconsistency_found) return;
    if (consistent(merger, left, right) == false) return;
    
    for(int i = 0; i < alphabet_size; ++i){
        if(l->pos(i) != 0 && r->pos(i) != 0){
            overlap += 1;
        }
        /*if(l->neg(i) != 0 && r->neg(i) != 0){
            overlap += 1;
        }*/
    }
};


int overlap_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  if(overlap > non_overlap) return overlap - non_overlap;
  return -1;
};

void overlap_driven::reset(state_merger *merger){
  inconsistency_found = false;
  non_overlap = 0;
  overlap = 0;
};


