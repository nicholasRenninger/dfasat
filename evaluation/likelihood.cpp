#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

/* Likelihood Ratio (LR), computes an LR-test (used in RTI) and uses the p-value as score and consistency */
void likelihoodratio::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if(right->accepting_paths < STATE_COUNT || left->accepting_paths < STATE_COUNT) return;
  
  double left_divider, right_divider, count_left, count_right, left_pool, right_pool, corr;
  
  corr = 1.0;
  for(int a = 0; a < alphabet_size; ++a){
    if(left->num_pos[a] >= SYMBOL_COUNT || right->num_pos[a] >= SYMBOL_COUNT)
      corr += CORRECTION;
  }
  
  left_divider = (double)left->accepting_paths + corr;
  right_divider = (double)right->accepting_paths + corr;
  
  left_pool = 0.0;
  right_pool = 0.0;
  for(int a = 0; a < alphabet_size; ++a){
    count_left  = (double)left->pos(a);
    count_right = (double)right->pos(a);
  
    if(count_left >= SYMBOL_COUNT || count_right >= SYMBOL_COUNT){
      count_left = count_left + CORRECTION; 
      count_right = count_right + CORRECTION; 
    
      if(count_left != 0.0)  loglikelihood_orig += count_left  * log(count_left  / left_divider);
      if(count_right != 0.0) loglikelihood_orig += count_right * log(count_right / right_divider);
      
      loglikelihood_merged += (count_left + count_right) * log((count_left + count_right) / (left_divider + right_divider));
      
      if(count_left > 0.0 && count_right > 0.0) extra_parameters = extra_parameters + 1;
    } else {
      left_pool += count_left;
      right_pool += count_right;
    }
  }
  left_pool = left_pool + CORRECTION;
  right_pool = right_pool + CORRECTION;
  if(left_pool != 0.0) loglikelihood_orig += left_pool * log(left_pool / left_divider);
  if(right_pool != 0.0) loglikelihood_orig += right_pool * log(right_pool / right_divider);
  if(left_pool != 0.0 || right_pool != 0.0) loglikelihood_merged += (left_pool + right_pool) * log((left_pool + right_pool) / (left_divider + right_divider));
};

bool likelihoodratio::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return false;
  if (extra_parameters == 0) return false;

  double test_statistic = 2.0 * (loglikelihood_orig - loglikelihood_merged);
  double p_value = gsl_cdf_chisq_P (test_statistic, (double)extra_parameters);
  
  if (p_value < CHECK_PARAMETER) return false;
  
  return true;
};

int likelihoodratio::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  double test_statistic = 2.0 * (loglikelihood_orig - loglikelihood_merged);
  double p_value = gsl_cdf_chisq_P (test_statistic, (double)extra_parameters);
  
  return (int)(p_value * 1000.0);  
};

void likelihoodratio::reset(state_merger *merger){
  inconsistency_found = false;
  loglikelihood_orig = 0;
  loglikelihood_merged = 0;
  extra_parameters = 0;
};

