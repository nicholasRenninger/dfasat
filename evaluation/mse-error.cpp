#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "mse-error.h"

//DerivedRegister<series_driven> series_driven::reg("series-driven");
REGISTER_DEF_TYPE(mse_error);
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

bool mse_error::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::consistent(merger,left,right) == false) return false;
    if(left->accepting_paths < STATE_COUNT || right->accepting_paths < STATE_COUNT) return true;

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

    if(mean_left - mean_right > CHECK_PARAMETER){ inconsistency_found = true; return false; }
    if(mean_right - mean_left > CHECK_PARAMETER){ inconsistency_found = true; return false; }

    //merge_error = merge_error + (error_total - (error_left + error_right));

    return true;
};

void mse_error::update_score(state_merger *merger, apta_node* left, apta_node* right){
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
};

int mse_error::compute_score(state_merger *merger, apta_node* left, apta_node* right){

    return 1000.0 - merge_error;

};

bool mse_error::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;

    return true;
};

void mse_error::reset(state_merger *merger ){
    inconsistency_found = false;
    merge_error = 0.0;
};
