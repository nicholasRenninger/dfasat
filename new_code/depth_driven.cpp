#include "evidence_driven.h"

/* RPNI like, merges shallow states (of lowest depth) first */
void depth_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if(depth == 0) depth = max(left->depth, right->depth);
};

int depth_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return depth;
};

void depth_driven::reset(state_merger *merger ){
  inconsistency_found = false;
  depth = 0;
};