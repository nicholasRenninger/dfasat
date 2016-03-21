#include "state_merger.h"
#include "evaluate.h"
#include "evaluation_factory.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>


            else {
                if(positive){
                    node->num_pos[c] = node->pos(c) + 1;
                    node->accepting_paths++;
                } else {
                    node->num_neg[c] = node->neg(c) + 1;
                    node->rejecting_paths++;
                }
            }
            //node->input_output
            if(occ >= 0)
                node->occs.push_front(occ);
                //node->child(c)->occs.push_front(occ);
        if(positive) node->num_accepting++;
        else node->num_rejecting++;

    num_pos = num_map();
    num_neg = num_map();

    num_accepting = 0;
    num_rejecting = 0;
    accepting_paths = 0;
    rejecting_paths = 0;


    left->num_accepting += right->num_accepting;
    left->num_rejecting += right->num_rejecting;
    left->accepting_paths += right->accepting_paths;
    left->rejecting_paths += right->rejecting_paths;

    //left->old_depth = left->depth;
    //left->depth = min(left->depth, right->depth);

    right->merge_point = left->conflicts.end();
    --(right->merge_point);
    left->conflicts.splice(left->conflicts.end(), right->conflicts);
    ++(right->merge_point);

    right->occ_merge_point = left->occs.end();
    --(right->occ_merge_point);
    left->occs.splice(left->occs.end(), right->occs);
    ++(right->occ_merge_point);

    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
        left->num_pos[(*it).first] = left->pos((*it).first) + (*it).second;
    }
    for(num_map::iterator it = right->num_neg.begin();it != right->num_neg.end(); ++it){
        left->num_neg[(*it).first] = left->neg((*it).first) + (*it).second;
    }



    left->num_accepting -= right->num_accepting;
    left->num_rejecting -= right->num_rejecting;
    left->accepting_paths -= right->accepting_paths;
    left->rejecting_paths -= right->rejecting_paths;

    //left->depth = left->old_depth;

    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
        left->num_pos[(*it).first] = left->pos((*it).first) - (*it).second;
    }
    for(num_map::iterator it = right->num_neg.begin();it != right->num_neg.end(); ++it){
        left->num_neg[(*it).first] = left->neg((*it).first) - (*it).second;
    }

    right->conflicts.splice(right->conflicts.begin(), left->conflicts, right->merge_point, left->conflicts.end());
    right->occs.splice(right->occs.begin(), left->occs, right->occ_merge_point, left->occs.end());


    inline int pos(int i){
        num_map::iterator it = num_pos.find(i);
        if(it == num_pos.end()) return 0;
        return (*it).second;
    }

    inline int neg(int i){
        num_map::iterator it = num_neg.find(i);
        if(it == num_neg.end()) return 0;
        return (*it).second;
    }


/* When is an APTA node a sink state?
 * sink states are not considered merge candidates
 *
 * accepting sink = only accept, accept now, accept afterwards
 * rejecting sink = only reject, reject now, reject afterwards */
bool is_low_count_sink(apta_node* node){
    node = node->find();
    return node->accepting_paths + node->num_accepting < STATE_COUNT;
}

bool is_accepting_sink(apta_node* node){
    node = node->find();
    for(num_map::iterator it = node->num_neg.begin();it != node->num_neg.end(); ++it){
        if((*it).second != 0) return false;
    }
    return node->num_rejecting == 0;
}

bool is_rejecting_sink(apta_node* node){
    node = node->find();
    for(num_map::iterator it = node->num_pos.begin();it != node->num_pos.end(); ++it){
        if((*it).second != 0) return false;
    }
    return node->num_accepting == 0;
}

bool is_single_event_sink(apta_node* node, int event){
    node = node->find();
    if(node->children.size() == 0) return true;
    if(node->children.size() != 1 || node->child(event) == 0) return false;
    apta_node* child = node->child(event);
    return is_single_event_sink(child, event);
}

bool is_single_event_sink(apta_node* node){
    node = node->find();
    if(node->children.size() == 0) return true;
    if(node->children.size() != 1) return false;
    int event = (*node->children.begin()).first;
    apta_node* child = node->child(event);
    return is_single_event_sink(child, event);
}

bool is_all_the_same_sink(apta_node* node){
    node = node->find();
    if(node->children.size() == 0) return true;
    if(node->children.size() != 1) return false;
    for(child_map::iterator it = node->children.begin(); it != node->children.end(); ++it){
        int event = (*it).first;
        apta_node* child = node->child(event);
        if(is_all_the_same_sink(child) == false) return false;
    }
    return true;
}

bool is_zero_occ_sink(apta_node* node){
    node = node->find();
    if(node->children.size() == 0) return true;
    //if(node->children.size() != 1) return false;
    if(node->occs.size() > 0) return false;
    int event = (*node->children.begin()).first;
    apta_node* child = node->child(event);
    return is_zero_occ_sink(child);
}

int get_event_type(apta_node* node){
    node = node->find();
    return (*node->children.begin()).first;
}

int sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    if (is_zero_occ_sink(node)) return 0;
    return -1;

    if (is_all_the_same_sink(node)) return 0;
    return -1;
    
    if (is_low_count_sink(node)) return 0;
    return -1;
    
    if (is_single_event_sink(node)) return get_event_type(node);
    return -1;
    
    if (is_accepting_sink(node)) return 0;
    if (is_rejecting_sink(node)) return 1;
    return -1;
}

bool sink_consistent(apta_node* node, int type){
    if(!USE_SINKS) return false;
    
    if(type == 0) return is_zero_occ_sink(node);
    return true;

    if(type == 0) return is_all_the_same_sink(node);
    return true;

    if(type == 0) return is_low_count_sink(node);
    return true;
    
    return sink_type(node) == type;
    
    if(type == 0) return node->rejecting_paths == 0 && node->num_rejecting == 0;
    if(type == 1) return node->accepting_paths == 0 && node->num_accepting == 0;
    return true;
}

int num_sink_types(){
    if(!USE_SINKS) return 0;
    return 1;
    return alphabet_size;
    return 1;
    return 2;
}

/* default evaluation, count number of performed merges */
bool evaluation_function::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(inconsistency_found) return false;
  
  if(left->num_accepting != 0 && right->num_rejecting != 0){ inconsistency_found = true; return false; }
  if(left->num_rejecting != 0 && right->num_accepting != 0){ inconsistency_found = true; return false; }
    
  return true;
};

void evaluation_function::update_score(state_merger *merger, apta_node* left, apta_node* right){
  num_merges += 1;
};

bool evaluation_function::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  return inconsistency_found == false;// && compute_score(merger, left, right) > LOWER_BOUND;
};

int evaluation_function::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return num_merges;
};

void evaluation_function::reset(state_merger *merger){
  inconsistency_found = false;
  num_merges = 0;
};

void evaluation_function::update(state_merger *merger){
};

void evaluation_function::initialize(state_merger *merger){
    compute_before_merge = false;
};

