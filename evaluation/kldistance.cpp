#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "state_merger.h"
#include "evaluate.h"
#include "kldistance.h"

#include "parameters.h"

REGISTER_DEF_DATATYPE(kl_data);
REGISTER_DEF_TYPE(kldistance);

bool kldistance::consistent(state_merger *merger, apta_node* left, apta_node* right){
    return count_driven::consistent(merger, left, right);
};

void kldistance::update_perplexity(apta_node* left, double count_left, double count_right, double left_divider, double right_divider){
    if(already_merged(left) == false){
        if(count_left != 0){
            perplexity += count_left * (count_left / left_divider) * log(count_left / left_divider);
            perplexity -= count_left * (count_left / left_divider)
                        * log((count_left + count_right) / (left_divider + right_divider));
        }
        if(count_right != 0){
            perplexity += count_right * (count_right / right_divider) * log(count_right / right_divider);
            perplexity -= count_right * (count_left / left_divider)
                        * log((count_left + count_right) / (left_divider + right_divider));
        }
    } else {
        if(count_left != 0){
            perplexity -= count_left * (count_left / left_divider) * log(count_left / left_divider);
            perplexity -= count_left * (count_left / left_divider)
                        * log((count_left + count_right) / (left_divider + right_divider));
        }
        if(count_right != 0){
            perplexity += count_right * (count_right / right_divider) * log(count_right / right_divider);
            perplexity -= count_right * (count_left / left_divider)
                        * log((count_left + count_right) / (left_divider + right_divider));
        }
    }
    if(count_left > 0.0 && count_right > 0.0) extra_parameters = extra_parameters + 1;
};

/* Kullback-Leibler divergence (KL), MDI-like, computes the KL value/extra parameters and uses it as score and consistency */
void kldistance::update_score(state_merger *merger, apta_node* left, apta_node* right){
    evaluation_function::update_score(merger, left, right);
    kl_data* l = (kl_data*) left->data;
    kl_data* r = (kl_data*) right->data;

    if(r->accepting_paths < STATE_COUNT || l->accepting_paths < STATE_COUNT) return;

    double left_divider, right_divider, corr;
    double left_count = 0.0;
    double right_count  = 0.0;

    corr = 1.0;
    for(int a = 0; a < alphabet_size; ++a){
    if(l->num_pos[a] >= SYMBOL_COUNT || r->num_pos[a] >= SYMBOL_COUNT)
      corr += CORRECTION;
    }

    left_divider = (double)l->accepting_paths + corr;
    right_divider = (double)r->accepting_paths + corr;
    
    int l1_pool = 0;
    int r1_pool = 0;
    int l2_pool = 0;
    int r2_pool = 0;
    int matching_right = 0;

    for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
        left_count = (*it).second;
        right_count = r->pos((*it).first);
        matching_right += right_count;
        
        if(left_count >= SYMBOL_COUNT && right_count >= SYMBOL_COUNT)
            update_perplexity(left, left_count, right_count, left_divider, right_divider);

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
    
    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT)
        update_perplexity(left, left_count, right_count, left_divider, right_divider);
    
    left_count = l2_pool;
    right_count = r2_pool;
    
    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT)
        update_perplexity(left, left_count, right_count, left_divider, right_divider);
};

bool kldistance::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return false;
  if (extra_parameters == 0) return false;

  if ((perplexity / extra_parameters) > CHECK_PARAMETER) return false;

  return true;
};

int kldistance::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  if (extra_parameters == 0) return -1;

  double val = (perplexity / extra_parameters);

  return 100000 - (int)(val * 100.0);
};

void kldistance::reset(state_merger *merger){
  alergia::reset(merger);
  inconsistency_found = false;
  perplexity = 0;
  extra_parameters = 0;
};

void kldistance::print_dot(FILE* output, state_merger* merger){
    count_driven::print_dot(output, merger);
}

