#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "state_merger.h"
#include "evaluate.h"
#include "depth-driven.h"
#include "alergia.h"

REGISTER_DEF_TYPE(alergia);

alergia_data::alergia_data() : count_data() {
    num_pos = num_map();
    num_neg = num_map();
};

void alergia_data::read(int type, int index, int length, int symbol, string data){
    count_data::read(type, index, length, symbol, data);
    if(type == 1){
        num_pos[c] = pos(c) + 1;
    } else {
        num_neg[c] = neg(c) + 1;
    }
};

void alergia_data::update(evaluation_data* right){
    count_data::update(right);
    alergia_data* other = (alergia_data*)right;
    for(num_map::iterator it = other->num_pos.begin();it != other->num_pos.end(); ++it){
        num_pos[(*it).first] = pos((*it).first) + (*it).second;
    }
    for(num_map::iterator it = other->num_neg.begin();it != other->num_neg.end(); ++it){
        num_neg[(*it).first] = neg((*it).first) + (*it).second;
    }
};

void alergia_data::undo(evaluation_data* right){
    count_data::undo(right);
    alergia_data* other = (alergia_data*)right;
    for(num_map::iterator it = other->num_pos.begin();it != other->num_pos.end(); ++it){
        num_pos[(*it).first] = pos((*it).first) - (*it).second;
    }
    for(num_map::iterator it = other->num_neg.begin();it != other->num_neg.end(); ++it){
        num_neg[(*it).first] = neg((*it).first) - (*it).second;
    }
};

/* ALERGIA, consistency based on Hoeffding bound, only uses positive (type=1) data, pools infrequent counts */
bool alergia::consistent(state_merger *merger, apta_node* left, apta_node* right){
    alergia_data* l = (alergia_data*) left->data;
    alergia_data* r = (alergia_data*) right->data;

    if(l->accepting_paths < STATE_COUNT || r->accepting_paths < STATE_COUNT) return true;

    double bound = sqrt(1.0 / (double)l->accepting_paths) + sqrt(1.0 / (double)r->accepting_paths);
    bound = bound * sqrt(0.5 * log(2.0 / CHECK_PARAMETER));
    
    int left_count = 0;
    int right_count  = 0;
    
    int l1_pool = 0;
    int r1_pool = 0;
    int l2_pool = 0;
    int r2_pool = 0;
    int matching_right = 0;

    for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
        left_count = (*it).second;
        right_count = r->pos((*it).first);
        matching_right += right_count;
        
        if(right_count >= SYMBOL_COUNT && right_count >= SYMBOL_COUNT) {
            double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
            if(gamma > bound){ inconsistency_found = true; return false; }
            if(gamma < -bound){ inconsistency_found = true; return false; }
        }

        if(right_count < SYMBOL_COUNT){
            l1_pool += left_count;
            r1_pool += right_count;
        }
        if(left_count < SYMBOL_COUNT) {
            l2_pool += left_count;
            r2_pool += right_count;
        }
    }
    r2_pool += r->accepting_paths - matching_right;
    
    left_count = l1_pool;
    right_count = r1_pool;
    
    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
        if(gamma > bound){ inconsistency_found = true; return false; }
        if(gamma < -bound){ inconsistency_found = true; return false; }
    }
    
    left_count = l2_pool;
    right_count = r2_pool;
    
    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
        if(gamma > bound){ inconsistency_found = true; return false; }
        if(gamma < -bound){ inconsistency_found = true; return false; }
    }
    
    left_count = l->num_accepting;
    right_count = r->num_accepting;

    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
        if(gamma > bound){ inconsistency_found = true; return false; }
        if(gamma < -bound){ inconsistency_found = true; return false; }
    }
    
    return true;
};
