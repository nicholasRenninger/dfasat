#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "process-mining.h"
#include "parameters.h"

REGISTER_DEF_DATATYPE(process_data);
REGISTER_DEF_TYPE(process_mining);

void process_data::print_state_label(iostream& output){
    for(set<int>::iterator it = done_tasks.begin(); it != done_tasks.end(); ++it){
        output << *it << "\n";
    }
};

bool process_mining::consistent(state_merger *merger, apta_node* left, apta_node* right){
    return overlap_driven::consistent(merger, left, right);
};

void process_mining::update_score(state_merger *merger, apta_node* left, apta_node* right){
    return overlap_driven::update_score(merger, left, right);
};

bool process_mining::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){

    return overlap_driven::compute_consistency(merger, left, right);
    process_data* l = (process_data*)left->data;
    process_data* r = (process_data*)right->data;
    
    if(l->done_tasks.size() != r->done_tasks.size()) return false;
    
    set<int>::iterator it  = l->done_tasks.begin();
    set<int>::iterator it2 = r->done_tasks.begin();
    
    while(it != l->done_tasks.end() && it2 != r->done_tasks.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else {
            return false;
        }
    }
    if(it != l->done_tasks.end() || it2 != r->done_tasks.end()){
        return false;
    }
    return true;
};

int process_mining::compute_score(state_merger* m, apta_node* left, apta_node* right){
    process_data* l = (process_data*)left->data;
    process_data* r = (process_data*)right->data;
    
    set<int>::iterator it  = l->done_tasks.begin();
    set<int>::iterator it2 = r->done_tasks.begin();
    
    while(it != l->done_tasks.end() && it2 != r->done_tasks.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else {
            return overlap_driven::compute_score(m, left, right);
        }
    }
    if(it != l->done_tasks.end() || it2 != r->done_tasks.end()){
        return overlap_driven::compute_score(m, left, right);
    }
    return LOWER_BOUND + overlap_driven::compute_score(m, left, right);
};

void process_mining::initialize(state_merger* m){
    for(merged_APTA_iterator it = merged_APTA_iterator(m->aut->root); *it != 0; ++it){
        apta_node* n = *it;
        apta_node* p = n;
        process_data* data = (process_data*)n->data;
        while(p->source != 0){
            data->done_tasks.insert(p->label);
            p = p->source;
        }
    }
};
