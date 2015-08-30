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
bool USE_SINKS = 0;
float MINIMUM_SCORE = 0;
float LOWER_BOUND = 0;

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

int sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    if (is_low_count_sink(node)) return 0;
    return -1;
    
    if (is_accepting_sink(node)) return 0;
    if (is_rejecting_sink(node)) return 1;
    return -1;
}

bool sink_consistent(apta_node* node, int type){
    if(!USE_SINKS) return false;
    
    if(type == 0) return is_low_count_sink(node);
    return true;
    
    if(type == 0) return node->rejecting_paths == 0 && node->num_rejecting == 0;
    if(type == 1) return node->accepting_paths == 0 && node->num_accepting == 0;
    return true;
}

int num_sink_types(){
    if(!USE_SINKS) return 0;
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
    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & left->pos((*it).first) == 0){
        inconsistency_found = true;
        return false;        
      }
    }
  }
  if(right->accepting_paths >= STATE_COUNT){
    for(num_map::iterator it = left->num_pos.begin();it != left->num_pos.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & right->pos((*it).first) == 0){
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
    if(left->pos(i) != 0 && right->pos(i) != 0){
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

/* Series driven, like overlap, but requires merges to be of the same depth */
bool series_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(evaluation_function::consistent(merger,left,right) == false) return false;
  if(left->depth != right->depth){ inconsistency_found = true; return false; }
  return true;
};

void series_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
  for(int i = 0; i < alphabet_size; ++i){
    if(left->pos(i) != 0 && right->pos(i) != 0){
      overlap += 1;
    }
  }
};

void series_driven::score_right(apta_node* right, int depth){
    if(right_dist[depth].find(right) != right_dist[depth].end()) return;
    right_dist[depth].insert(right);
    if(depth < right_dist.size() - 1){
        for(child_map::iterator it = right->children.begin(); it != right->children.end(); ++it){
            score_right((*it).second->find(), depth + 1);
        }
    }
};

void series_driven::score_left(apta_node* left, int depth){
    if(left_dist[depth].find(left) != left_dist[depth].end()) return;
    left_dist[depth].insert(left);
    if(depth < left_dist.size() - 1){
        for(child_map::iterator it = left->children.begin(); it != left->children.end(); ++it){
            score_left((*it).second->find(), depth + 1);
        }
    }
};

int series_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    if(left->depth != right->depth){ return -1; }
    score_right(right,0);
    score_left(left,0);
    
    int tests_passed = 0;
    int tests_failed = 0;
    
    double distance = 0.0;

    for(int i = 0; i < merger->aut->max_depth; ++i){
        int total_left = 0;
        int total_right = 0;
        for(state_set::iterator it = left_dist[i].begin(); it != left_dist[i].end(); ++it){
            apta_node* node = *it;
            total_left += node->accepting_paths;
        }
        for(state_set::iterator it = right_dist[i].begin(); it != right_dist[i].end(); ++it){
            apta_node* node = *it;
            total_right += node->accepting_paths;
        }
        //cerr << "total " << total_left << " " << total_right << endl;

        if(total_left >= STATE_COUNT && total_right >= STATE_COUNT){
            double bound = sqrt(1.0 / (double)total_left) + sqrt(1.0 / (double)total_right);
            bound = bound * sqrt(0.5 * log(2.0 / CHECK_PARAMETER));
            
            for(int a = 0; a < alphabet_size; ++a){
                int count_left = 0;
                int count_right = 0;
                for(state_set::iterator it = left_dist[i].begin(); it != left_dist[i].end(); ++it){
                    apta_node* node = *it;
                    count_left += node->pos(a);
                }
                for(state_set::iterator it = right_dist[i].begin(); it != right_dist[i].end(); ++it){
                    apta_node* node = *it;
                    count_right += node->pos(a);
                }
                if(count_left > 0 || count_right > 0){
                    double gamma = ((double)count_left / (double)total_left) - ((double)count_right / (double)total_right);
                    if(gamma < 0.0) gamma = -gamma;
                    distance += gamma;
                }
                //cerr << "dist-" << a << "  " << count_left << " " << count_right << endl;
                if(count_left >= SYMBOL_COUNT || count_right >= SYMBOL_COUNT){
                    double gamma = ((double)count_left / (double)total_left) - ((double)count_right / (double)total_right);
                    if(gamma < 0.0) gamma = -gamma;
                    if(gamma > bound){ tests_failed += 1; }
                    else { tests_passed += 1; }
                }
            }
        } else {
            for(int a = 0; a < alphabet_size; ++a){
                int count_left = 0;
                int count_right = 0;
                for(state_set::iterator it = left_dist[i].begin(); it != left_dist[i].end(); ++it){
                    apta_node* node = *it;
                    count_left += node->pos(a);
                }
                for(state_set::iterator it = right_dist[i].begin(); it != right_dist[i].end(); ++it){
                    apta_node* node = *it;
                    count_right += node->pos(a);
                }
                if(count_left > 0 || count_right > 0){
                    double gamma = ((double)count_left / (double)total_left) - ((double)count_right / (double)total_right);
                    if(gamma < 0.0) gamma = -gamma;
                    distance += gamma;
                }
            }
        }
    }
    
    if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) distance = distance / 2;
    
    //cerr << "passed: " << tests_passed << "  failed: " << tests_failed << endl;
    
    if(tests_failed == 0)
        return 100 * ((double)merger->aut->max_depth - distance);
    return -1;
};

void series_driven::initialize(state_merger *merger){
    left_dist = vector< state_set >(merger->aut->max_depth);
    right_dist = vector< state_set >(merger->aut->max_depth);
    for(int i = 0; i < merger->aut->max_depth; ++i){
        left_dist[i] = state_set();
        right_dist[i] = state_set();
    }
    compute_before_merge = true;
};

void series_driven::reset(state_merger *merger){
  for(int i = 0; i < merger->aut->max_depth; ++i){
    left_dist[i].clear();
    right_dist[i].clear();
  }
  inconsistency_found = false;
  overlap = 0;
};

/* Metric driven merging, addition by chrham */
bool metric_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(evaluation_function::consistent(merger,left,right) == false) return false;

  if(left->accepting_paths >= STATE_COUNT){
    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & left->pos((*it).first) == 0){
        inconsistency_found = true;
        return false;        
      }
    }
  }
  if(right->accepting_paths >= STATE_COUNT){
    for(num_map::iterator it = left->num_pos.begin();it != left->num_pos.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & right->pos((*it).first) == 0){
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
    if(left->pos(i) != 0 && right->pos(i) != 0){
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
      if(left->pos(i) >= SYMBOL_COUNT || right->pos(i) >= SYMBOL_COUNT){
        double gamma = 0.0;
        if( ((double)left->pos(i) / (double)left->accepting_paths) >
            ((double)right->pos(i) / (double)right->accepting_paths) )
          gamma = ((double)left->pos(i) / (double)left->accepting_paths)
                - ((double)right->pos(i) / (double)right->accepting_paths);
        else
          gamma = ((double)right->pos(i) / (double)right->accepting_paths)
                - ((double)left->pos(i) / (double)left->accepting_paths);
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
