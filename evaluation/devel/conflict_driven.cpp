#include "state_merger.h"
#include "evaluate.h"
#include "evaluation_factory.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>


/* default evaluation, count number of performed merges */
bool conflict_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(inconsistency_found) return false;
  
  if(left->num_accepting != 0 && right->num_rejecting != 0){ inconsistency_found = true; return false; }
  if(left->num_rejecting != 0 && right->num_accepting != 0){ inconsistency_found = true; return false; }
    
  return true;
};

void conflict_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  num_merges += 1;
};

bool conflict_driven::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  return inconsistency_found == false && compute_score(merger, left, right) > LOWER_BOUND;
};

int conflict_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return num_merges;
};

void conflict_driven::reset(state_merger *merger){
  inconsistency_found = false;
  num_merges = 0;
};

void conflict_driven::update(state_merger *merger){
};

void conflict_driven::initialize(state_merger *merger){
    compute_before_merge = false;
};

