#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

/* Evidence driven state merging, count number of pos-pos and neg-neg merges */
void evidence_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if(left->num_accepting > 0 && right->num_accepting > 0) num_pos += 1;
  if(left->num_rejecting > 0 && right->num_rejecting > 0) num_neg += 1;
};

int evidence_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return num_pos + num_neg;
};

void evidence_driven::reset(state_merger *merger){
  inconsistency_found = false;
  num_pos = 0;
  num_neg = 0;
};
