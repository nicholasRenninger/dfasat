#include "state_merger.h"
#include "conflict-driven.h"
#include "evaluation_factory.h"
#include "num_count.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

REGISTER_DEF_DATATYPE(conflict_data);
REGISTER_DEF_TYPE(conflict_driven);

void conflict_bits_data::update(evaluation_data* right){
    count_data::update(right);
    conflict_data* r = (conflict_data*)right;
    
    r->undo_info = r-conflicts - conflicts;
    conflicts = conflicts + r->undo_info;
    
};

void conflict_bits_data::undo(evaluation_data* right){
    count_data::undo(right);
    
    conflicts = conflicts + r->undo_info;
};

int conflict_driven_bits::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    conflict_data* r = (conflict_data*)right->data;
    int val = r->conflicts.size() - r->undo_info.size();
    if (val > 0) return val;
    return 0;
};

void conflict_driven_bits::reset(state_merger *merger){
  count_driven::reset(merger);
};

void conflict_driven_bits::initialize(state_merger *merger){
    count_driven::initialize(merger);
    compute_before_merge = false;
    
    for(merged_APTA_iterator it = merged_APTA_iterator(merger->aut->root); *it != 0; ++it){
        apta_node* n1 = *it;

        for(merged_APTA_iterator it2 = it; *it2 != 0; ++it2){
            if(*it == *it2) continue;
            apta_node* n2 = *it2;
            
            if(merger->merge_test(n1,n2) == false){
                conflict_data* d1 = (conflict_data*)n1->data;
                conflict_data* d2 = (conflict_data*)n2->data;
                d1->conflicts.insert(n2->number);
                d2->conflicts.insert(n1->number);
            }
        }
    }
};

