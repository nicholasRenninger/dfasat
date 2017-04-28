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

void conflict_data::update(evaluation_data* right){
    count_data::update(right);

    conflict_data* r = (conflict_data*)right;
    
    set<int>::iterator it  = conflicts.begin();
    set<int>::iterator it2 = r->conflicts.begin();
    r->undo_info.clear();
    
    while(it != conflicts.end() && it2 != r->conflicts.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else if(*it < *it2){
            it++;
        } else {
            r->undo_info.insert(*it2);
            it2++;
        }
    }
    while(it2 != r->conflicts.end()){
        r->undo_info.insert(*it2);
        it2++;
    }
    
    for(it = r->undo_info.begin(); it != r->undo_info.end(); ++it){
        conflicts.insert(*it);
    }
};

void conflict_data::undo(evaluation_data* right){
    count_data::undo(right);
    
    conflict_data* r = (conflict_data*)right;
    
    for(set<int>::iterator it = r->undo_info.begin(); it != r->undo_info.end(); ++it){
        conflicts.erase(*it);
    }
};

int conflict_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    conflict_data* r = (conflict_data*)right->data;
    int val = r->conflicts.size() - r->undo_info.size();
    //cerr << r->conflicts.size() << endl;
    if (val > 0) return val;
    return 0;
};

void conflict_driven::reset(state_merger *merger){
  count_driven::reset(merger);
};

void conflict_driven::initialize(state_merger *merger){
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
        //cerr << n1->number << " " << ((conflict_data*)n1->data)->conflicts.size() << endl;
    }
};

