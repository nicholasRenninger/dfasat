#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "full-overlap-driven.h"
#include "parameters.h"

REGISTER_DEF_DATATYPE(full_overlap_data);
REGISTER_DEF_TYPE(full_overlap_driven);

/* Overlap driven, count overlap in positive transitions, used in Stamina winner */
bool full_overlap_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(count_driven::consistent(merger, left, right) == false){ inconsistency_found = true; return false; }
    
    full_overlap_data* l = (full_overlap_data*) left->data;
    full_overlap_data* r = (full_overlap_data*) right->data;

    if(l->accepting_paths >= STATE_COUNT){
        for(num_map::iterator it = r->num_pos.begin(); it != r->num_pos.end(); ++it){
            if((*it).second >= SYMBOL_COUNT && l->pos((*it).first) == 0){
                inconsistency_found = true;
                return false;
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

void full_overlap_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
    full_overlap_data* l = (full_overlap_data*) left->data;
    full_overlap_data* r = (full_overlap_data*) right->data;
    
    if (inconsistency_found) return;
    //if (consistent(merger, left, right) == false) return;
    
    if (l->accepting_paths != 0 && r->accepting_paths != 0) overlap++;
    
    /*for(int i = 0; i < alphabet_size; ++i){
        if(l->pos(i) != 0 && r->pos(i) != 0){
            overlap += 1;
        }
        if(l->neg(i) != 0 && r->neg(i) != 0){
            overlap += 1;
        }
    }*/
};


int full_overlap_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return overlap;
};

void full_overlap_driven::reset(state_merger *merger){
  inconsistency_found = false;
  overlap = 0;
  compute_before_merge = false;
};


