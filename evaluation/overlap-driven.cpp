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
    if(count_driven::consistent(merger, left, right) == false){
        inconsistency_found = true;
        return false;
    }
    /*
    if(left->label != right->label){
        inconsistency_found = true;
        return false;
    }
    */
    
    overlap_data* l = (overlap_data*) left->data;
    overlap_data* r = (overlap_data*) right->data;
    
    /*if(l->num_accepting != 0 && r->num_accepting == 0){
        inconsistency_found = true; return false;
    }
    if(r->num_accepting != 0 && l->num_accepting == 0){
        inconsistency_found = true; return false;
    }*/

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
    if(l->num_accepting >= SYMBOL_COUNT && r->num_accepting == 0){
        inconsistency_found = true;
        return false;
    }
    if(l->num_accepting == 0 && r->num_accepting >= SYMBOL_COUNT){
        inconsistency_found = true;
        return false;
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
            if(l->pos(i) > r->pos(i)) overlap += r->pos(i);
            if(r->pos(i) > l->pos(i)) overlap += l->pos(i);
        } else {
            if(l->pos(i) != 0){
                overlap -= l->pos(i);
            }
            if(r->pos(i) != 0){
                overlap -= r->pos(i);
            }
        }
        /*if(l->neg(i) != 0 && r->neg(i) != 0){
            overlap += 1;
        }*/
    }
    if(l->num_accepting != 0 && r->num_accepting != 0){
        if(l->num_accepting > r->num_accepting) overlap += r->num_accepting;
        if(r->num_accepting > l->num_accepting) overlap += l->num_accepting;
    } else {
        if(l->num_accepting != 0){
            overlap -= l->num_accepting;
        }
        if(r->num_accepting != 0){
            overlap -= r->num_accepting;
        }
    }
};


int overlap_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  if(overlap > 0) return overlap;
  return 0;
};

void overlap_driven::reset(state_merger *merger){
  inconsistency_found = false;
  overlap = 0;
};

void overlap_data::print_transition_label(iostream& output, int symbol){
    output << (num_pos[symbol]+num_neg[symbol]);
};

