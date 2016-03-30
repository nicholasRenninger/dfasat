#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "depth-driven.h"

//DerivedRegister<series_driven> series_driven::reg("series-driven");
REGISTER_DEF_TYPE(depth_driven);

void count_data::read(int type, int index, int length, int symbol, string data){
    depth = length - index;
};

/* RPNI like, merges shallow states (of lowest depth) first */
int depth_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    return max(left->data->depth, right->data->depth);
};

void recur_depth(apta_node* state, state_set& states, int depth){
    if(states.find(state) != states.end()) return;
    state->data->depth = depth;
    states.insert(state);
    for(child_map::iterator it = state->children.begin();it != state->children.end(); ++it){
        apta_node* child = (*it).second;
        if(child != 0) recur_depth(child, states, depth + 1);
    }
}

void depth_driven::reset(state_merger *merger){
    state_set visited;
    recur_depth(merger->aut->root, visited, 1);
    inconsistency_found = false;
};
