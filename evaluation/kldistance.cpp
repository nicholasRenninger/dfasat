#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>


/* Kullback-Leibler divergence (KL), MDI-like, computes the KL value/extra parameters and uses it as score and consistency */
void kldistance::update_score(state_merger *merger, apta_node* left, apta_node* right){
  depth_driven::update_score(merger, left, right);

  if(right->accepting_paths < STATE_COUNT || left->accepting_paths < STATE_COUNT) return;
  
  double left_divider, right_divider, count_left, count_right, left_pool, right_pool, corr;
  
  corr = 1.0;
  for(int a = 0; a < alphabet_size; ++a){
    if(left->num_pos[a] >= SYMBOL_COUNT
       || right->num_pos[a] >= SYMBOL_COUNT)
      corr += CORRECTION;
  }
  
  left_divider = (double)left->accepting_paths + corr;
  right_divider = (double)right->accepting_paths + corr;
  
  left_pool = 0.0;
  right_pool = 0.0;
  for(int a = 0; a < alphabet_size; ++a){
    count_left = (double)left->pos(a);
    count_right = (double)right->pos(a);
  
    if(count_left >= SYMBOL_COUNT || count_right >= SYMBOL_COUNT){
      count_left = count_left + CORRECTION; 
      count_right = count_right + CORRECTION; 
    
        if(count_left != 0){
          perplexity += count_left * (count_left / left_divider) * log(count_left / left_divider);
          perplexity -= count_left * (count_left / left_divider) 
                      * log((count_left + count_right) / (left_divider + right_divider));
        }
        if(count_right != 0){
          perplexity += count_right * (count_right / right_divider) * log(count_right / right_divider);
          perplexity -= count_right * (count_right / right_divider) 
                      * log((count_left + count_right) / (left_divider + right_divider));
        }
        if(count_left > 0.0 && count_right > 0.0) extra_parameters = extra_parameters + 1;
    } else {
      left_pool += count_left;
      right_pool += count_right;
    }
  }
  left_pool = left_pool + CORRECTION;
  right_pool = right_pool + CORRECTION;
  if(left_pool != 0.0){
    perplexity += left_pool * (left_pool / left_divider) * log(left_pool / left_divider);
    perplexity -= left_pool * (left_pool / left_divider) * log((left_pool + right_pool) / (left_divider + right_divider));
  }
  if(right_pool != 0.0){
    perplexity += right_pool * (right_pool / right_divider) * log(right_pool / right_divider);
    perplexity -= right_pool * (right_pool / right_divider) * log((left_pool + right_pool) / (left_divider + right_divider));
  }
};

bool kldistance::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return false;
  if (extra_parameters == 0) return false;

  if ((perplexity / extra_parameters) > CHECK_PARAMETER) return false;
  
  return true;
};

void kldistance::reset(state_merger *merger){
  depth_driven::reset(merger);
  inconsistency_found = false;
  perplexity = 0;
  extra_parameters = 0;
};


