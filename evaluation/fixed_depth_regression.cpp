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
/* RPNI like, merges shallow states (of lowest depth) first */
/*void depth_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if(depth == 0) depth = max(left->depth, right->depth);
};

int depth_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return depth;
};

void depth_driven::reset(state_merger *merger ){
  inconsistency_found = false;
  depth = 0;
};*/
            //node->input_output
            if(occ >= 0)
                node->occs.push_front(occ);
                //node->child(c)->occs.push_front(occ);


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

bool depth_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::consistent(merger,left,right) == false) return false;
    if(left->accepting_paths < STATE_COUNT || right->accepting_paths < STATE_COUNT) return true;

/*


    double max_left = -1;
    double max_right =-1;
    int left_symbol = -1;
    int right_symbol = -1;

    for(int i = 0; i < alphabet_size; ++i){

        if( (double)left->pos(i) > max_left ) {
        max_left = (double)left->pos(i);
        left_symbol = i;
      }

        if ((double)right->pos(i) > max_right) {
        max_right = (double)right->pos(i);
        right_symbol = i;
      }

    }

    if(left_symbol != right_symbol) {
        inconsistency_found = true;
        return false;
    }
    return true;*/

    double error_left = 0.0;
    double error_right = 0.0;
    double error_total = 0.0;
    double mean_left = 0.0;
    double mean_right = 0.0;
    double mean_total = 0.0;

    for(double_list::iterator it = left->occs.begin(); it != left->occs.end(); ++it){
        mean_left = mean_left + (double)*it;
    }
    for(double_list::iterator it = right->occs.begin(); it != right->occs.end(); ++it){
        mean_right = mean_right + (double)*it;
    }
    mean_total = (mean_left + mean_right) / ((double)left->occs.size() + (double)right->occs.size());
    mean_right = mean_right / (double)right->occs.size();
    mean_left = mean_left / (double)left->occs.size();

    /*for(double_list::iterator it = left->occs.begin(); it != left->occs.end(); ++it){
        error_left = error_left + ((mean_left - (double)*it)*(mean_left - (double)*it));
        error_total = error_total + ((mean_total - (double)*it)*(mean_total - (double)*it));
    }
    for(double_list::iterator it = right->occs.begin(); it != right->occs.end(); ++it){
        error_right = error_right + ((mean_right - (double)*it)*(mean_right - (double)*it));
        error_total = error_total + ((mean_total - (double)*it)*(mean_total - (double)*it));
    }*/

    //error_right = error_right / ((double)left->occs.size() + (double)right->occs.size());
    //error_left = error_left / ((double)left->occs.size() + (double)right->occs.size());
    //error_total = (error_total) / ((double)left->occs.size() + (double)right->occs.size());

    //if(error_total - (error_left + error_right) > CHECK_PARAMETER){ inconsistency_found = true; return false; }
    if(mean_left - mean_right > CHECK_PARAMETER){ inconsistency_found = true; return false; }
    if(mean_right - mean_left > CHECK_PARAMETER){ inconsistency_found = true; return false; }

    //merge_error = merge_error + (error_total - (error_left + error_right));

    return true;
};

void depth_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
    if (inconsistency_found) return;
    if (consistent(merger, left, right) == false) return;
    double error_left = 0.0;
    double error_right = 0.0;
    double error_total = 0.0;
    double mean_left = 0.0;
    double mean_right = 0.0;
    double mean_total = 0.0;

    for(double_list::iterator it = left->occs.begin(); it != left->occs.end(); ++it){
        mean_left = mean_left + (double)*it;
    }
    for(double_list::iterator it = right->occs.begin(); it != right->occs.end(); ++it){
        mean_right = mean_right + (double)*it;
    }
    mean_total = (mean_left + mean_right) / ((double)left->occs.size() + (double)right->occs.size());
    mean_right = mean_right / (double)right->occs.size();
    mean_left = mean_left / (double)left->occs.size();

    for(double_list::iterator it = left->occs.begin(); it != left->occs.end(); ++it){
        error_left = error_left + ((mean_left - (double)*it)*(mean_left - (double)*it));
        error_total = error_total + ((mean_total - (double)*it)*(mean_total - (double)*it));
    }
    for(double_list::iterator it = right->occs.begin(); it != right->occs.end(); ++it){
        error_right = error_right + ((mean_right - (double)*it)*(mean_right - (double)*it));
        error_total = error_total + ((mean_total - (double)*it)*(mean_total - (double)*it));
    }

    error_right = error_right / ((double)left->occs.size() + (double)right->occs.size());
    error_left = error_left / ((double)left->occs.size() + (double)right->occs.size());
    error_total = (error_total) / ((double)left->occs.size() + (double)right->occs.size());

    merge_error = merge_error + (error_total - (error_left + error_right));

    return;
    /*
    double max_left = -1;
    double max_right =-1;
    double max_total = -1;
    int left_symbol = -1;
    int right_symbol = -1;

    for(int i = 0; i < alphabet_size; ++i){
        if( (double)left->pos(i) + right->pos(i) > max_total ) {
            max_total = (double)left->pos(i) + (double)right->pos(i);
        }

        if( (double)left->pos(i) > max_left ) {
            max_left = (double)left->pos(i);
            left_symbol = i;
        }

        if ((double)right->pos(i) > max_right) {
            max_right = (double)right->pos(i);
            right_symbol = i;
        }
    }

    int error_left  = left->accepting_paths  - max_left;
    int error_right = right->accepting_paths - max_right;
    int error_total = left->accepting_paths + right->accepting_paths - max_total;

    //if(left_symbol == right_symbol) merge_error = merge_error + 1;
    merge_error = merge_error + (error_total - error_left - error_right);*/
};

int depth_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    //if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) merge_error = merge_error / 2.0;
    //if(merge_error > 1) return -1;
    return 1000.0 - merge_error;
    //return merge_error;
};

bool depth_driven::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;
    //if(merge_error > 0) return false;
    return true;
};

void depth_driven::reset(state_merger *merger ){
    inconsistency_found = false;
    merge_error = 0.0;
};
