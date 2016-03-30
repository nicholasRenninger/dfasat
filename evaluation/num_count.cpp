#include "state_merger.h"
#include "evaluate.h"
#include "evaluation_factory.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

REGISTER_DEF_TYPE(count_driven);

count_data::count_data(){
    num_accepting = 0;
    num_rejecting = 0;
    accepting_paths = 0;
    rejecting_paths = 0;
};

void count_data::read(int type, int index, int length, int symbol, string data){
    if(type == 1){
        accepting_paths++;
        if(length == index+1){
            num_accepting++;
        }
    } else {
        rejecting_paths++;
        if(length == index+1){
            num_rejecting++;
        }
    }
};

void count_data::update(evaluation_data* right){
    count_data other = (count_data*)right;
    num_accepting += other->num_accepting;
    num_rejecting += other->num_rejecting;
    accepting_paths += other->accepting_paths;
    rejecting_paths += other->rejecting_paths;
};

void count_data::undo(evaluation_data* right){
    count_data other = (count_data*)right;
    num_accepting -= other->num_accepting;
    num_rejecting -= other->num_rejecting;
    accepting_paths -= other->accepting_paths;
    rejecting_paths -= other->rejecting_paths;
};

/* default evaluation, count number of performed merges */
bool count_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(inconsistency_found) return false;
  
    count_data* l = (count_data*) left->data;
    count_data* r = (count_data*) right->data;
    
    if(l->num_accepting != 0 && r->num_rejecting != 0){ inconsistency_found = true; return false; }
    if(l->num_rejecting != 0 && r->num_accepting != 0){ inconsistency_found = true; return false; }
    
    return true;
};

void count_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  num_merges += 1;
};

int count_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return num_merges;
};

void count_driven::reset(state_merger *merger){
  inconsistency_found = false;
  num_merges = 0;
};


