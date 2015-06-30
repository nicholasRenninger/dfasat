#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

int STATE_COUNT = 0;
int SYMBOL_COUNT = 0;
float CORRECTION = 0.0;
float CHECK_PARAMETER = 0.0;


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
  return inconsistency_found == false;
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
};

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

/* Overlap driven, count overlap in positive transitions, used in Stamina winner */
bool overlap_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(evaluation_function::consistent(merger,left,right) == false) return false;

  if(left->accepting_paths >= STATE_COUNT){
    for(int i = 0; i < alphabet_size; ++i){
      if(right->num_pos[i] >= SYMBOL_COUNT & left->num_pos[i] == 0){
        inconsistency_found = true;
        return false;        
      }
    }
  }
  if(right->accepting_paths >= STATE_COUNT){
    for(int i = 0; i < alphabet_size; ++i){
      if(left->num_pos[i] >= SYMBOL_COUNT & right->num_pos[i] == 0){
        inconsistency_found = true;
        return false;
      }
    }
  }
  return true;
};

void overlap_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return;
  if (consistent(merger, left, right) == false) return;
  
  for(int i = 0; i < alphabet_size; ++i){
    if(left->num_pos[i] != 0 && right->num_pos[i] != 0){
      overlap += 1;
    }
  }
};

bool overlap_driven::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;
    return true;
};

int overlap_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return overlap;
};

void overlap_driven::reset(state_merger *merger){
  inconsistency_found = false;
  overlap = 0;
};

/* Metric driven merging, addition by chrham */
bool metric_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(evaluation_function::consistent(merger,left,right) == false) return false;

  if(left->accepting_paths >= STATE_COUNT){
    for(int i = 0; i < alphabet_size; ++i){
      if(right->num_pos[i] >= SYMBOL_COUNT & left->num_pos[i] == 0){
        inconsistency_found = true;
        return false;        
      }
    }
  }
  if(right->accepting_paths >= STATE_COUNT){
    for(int i = 0; i < alphabet_size; ++i){
      if(left->num_pos[i] >= SYMBOL_COUNT & right->num_pos[i] == 0){
        inconsistency_found = true;
        return false;
      }
    }
  }
  return true;
};

// what's a good score?
void metric_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return;
  if (consistent(merger, left, right) == false) return;
  
  for(int i = 0; i < alphabet_size; ++i){
    if(left->num_pos[i] != 0 && right->num_pos[i] != 0){
      overlap += 1;
    }
  }
};

bool metric_driven::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;
    return true;
};

int metric_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return overlap;
};

void metric_driven::reset(state_merger *merger){
  inconsistency_found = false;
  overlap = 0;
};

/* end addition */

/* ALERGIA, consistency based on Hoeffding bound, merges depth driven */
bool alergia::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(depth_driven::consistent(merger,left,right) == false) return false;
  
  if(left->accepting_paths >= STATE_COUNT && right->accepting_paths >= STATE_COUNT){
    double bound = sqrt(1.0 / (double)left->accepting_paths) 
                 + sqrt(1.0 / (double)right->accepting_paths);
    bound = bound * sqrt(0.5 * log(2.0 / CHECK_PARAMETER));
    
    for(int i = 0; i < alphabet_size; ++i){
      if(left->num_pos[i] >= SYMBOL_COUNT || right->num_pos[i] >= SYMBOL_COUNT){
        double gamma = 0.0;
        if( ((double)left->num_pos[i] / (double)left->accepting_paths) > 
            ((double)right->num_pos[i] / (double)right->accepting_paths) )
          gamma = ((double)left->num_pos[i] / (double)left->accepting_paths)
                - ((double)right->num_pos[i] / (double)right->accepting_paths);
        else
          gamma = ((double)right->num_pos[i] / (double)right->accepting_paths)
                - ((double)left->num_pos[i] / (double)left->accepting_paths);
        if(gamma > bound){ inconsistency_found = true; return false; }
      }
    }
  }
  return true;
};

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
    count_left  = (double)left->num_pos[a];
    count_right = (double)right->num_pos[a];
  
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

/* Akaike Information Criterion (AIC), computes the AIC value and uses it as score, AIC increases are inconsistent */
bool aic::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return false;
  if (extra_parameters == 0) return false;

  double val = -2.0 * ( extra_parameters - (loglikelihood_orig - loglikelihood_merged) );
  
  if(val <= 0) return false;
  
  return true;
};

int aic::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  double val = -2.0 * ( extra_parameters - (loglikelihood_orig - loglikelihood_merged) );
  
  return (int)val * 100.0;
};

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
    count_left = (double)left->num_pos[a];
    count_right = (double)right->num_pos[a];
  
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

/* COMMENTED OUT, might be useful when computing distance from APTA distribution
 * as I understand it, the MDI algorithm computes it incremental from previous dist*/
/*
  init_perplexity = 0.0;
  init_parameters = 0;
  state_set states = merger->aut->get_states();
  for(state_set::iterator it = states.begin(); it != states.end(); ++it){
    double divider, count;
    divider = (double)(*it)->accepting_paths;
    for(int a = 0; a < alphabet_size; ++a){
      count = (double)(*it)->num_pos[a];
      if(count > 0.0){
        init_perplexity += count * (count / divider) * log(count / divider);
        init_parameters += 1;
      }
    }
  }
*/
