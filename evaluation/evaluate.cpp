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

int get_event_type(apta_node* node){
    node = node->find();
    return (*node->children.begin()).first;
}

int sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    //if (is_single_event_sink(node)) return get_event_type(node);
    //return -1;

    if (is_low_count_sink(node)) return 0;
    return -1;
    
    if (is_accepting_sink(node)) return 0;
    if (is_rejecting_sink(node)) return 1;
    return -1;
}

bool sink_consistent(apta_node* node, int type){
    if(!USE_SINKS) return false;
    
    //return sink_type(node) == type;

    if(type == 0) return is_low_count_sink(node);
    return true;
    
    if(type == 0) return node->rejecting_paths == 0 && node->num_rejecting == 0;
    if(type == 1) return node->accepting_paths == 0 && node->num_accepting == 0;
    return true;
}

int num_sink_types(){
    if(!USE_SINKS) return 0;
    //return alphabet_size;
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

bool depth_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::consistent(merger,left,right) == false) return false;
    if(left->accepting_paths < STATE_COUNT || right->accepting_paths < STATE_COUNT) return true;
    /*if(left->depth < 3 || right->depth < 3){
        inconsistency_found = true;
        return false;
    }*/
   
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
    for(num_map::iterator it = right->num_neg.begin();it != right->num_neg.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & left->neg((*it).first) == 0){
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
    for(num_map::iterator it = left->num_neg.begin();it != left->num_neg.end(); ++it){
      if((*it).second >= SYMBOL_COUNT & right->neg((*it).first) == 0){
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
    if(left->neg(i) != 0 && right->neg(i) != 0){
      overlap += 1;
    }
  }
};

bool overlap_driven::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;
    if(left->depth != right->depth){ inconsistency_found = true; return false; }
    return true;
};

int overlap_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) overlap = overlap * 2;
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

/*
  int count_left = left->accepting_paths;
  int count_right = right->accepting_paths;
  
  int total_left = left->accepting_paths  + left->rejecting_paths;
  int total_right = right->accepting_paths + right->rejecting_paths;

  double observed = (double)count_left;
  double expected = (double)total_left * ((double)(count_left+count_right) / ((double)total_left + total_right));
  double diff = observed - expected;
  if(diff < 0.0) diff = - diff;
  if (diff > 5.0){
    inconsistency_found = true; return false;
  }
  
  observed = (double)count_right;
  expected = (double)total_right * ((double)(count_left+count_right) / ((double)total_left + total_right));
  diff = observed - expected;
  if(diff < 0.0) diff = - diff;
  if (diff > 5.0){
    inconsistency_found = true; return false;
  }
*/
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
    double gamma;
    
    int num_tests = 0;

    for(int i = 0; i < merger->aut->max_depth; ++i){
  
        for(int a = 0; a < alphabet_size; ++a){
            int count_left = 0;
            int count_right = 0;
            int total_left = 0;
            int total_right = 0;
            for(state_set::iterator it = left_dist[i].begin(); it != left_dist[i].end(); ++it){
                apta_node* node = *it;
                count_left += node->pos(a) + node->neg(a);
                total_left += node->accepting_paths + node->rejecting_paths;
            }
            for(state_set::iterator it = right_dist[i].begin(); it != right_dist[i].end(); ++it){
                apta_node* node = *it;
                count_right += node->pos(a) + node->neg(a);
                total_right += node->accepting_paths + node->rejecting_paths;
            }
            
            if(total_left == 0 || total_right == 0) continue;
            
            double observed = (double)count_left;
            double expected = (double)total_left * ((double)(count_left+count_right) / ((double)total_left + total_right));
            double diff = observed - expected;
            if(diff < 0.0) diff = - diff;
            if (diff > 5.0){
                //cerr << diff << " " << observed << " " << expected << " " << count_left << "/" << total_left << " " << count_right << "/" << total_right << endl;
                return -1;
            }
            if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) diff = diff / 2.0;
            distance += 5.0 - diff;
            
            observed = (double)count_right;
            expected = (double)total_right * ((double)(count_left+count_right) / ((double)total_left + total_right));
            diff = observed - expected;
            if(diff < 0.0) diff = - diff;
            if (diff > 5.0){
                //cerr << diff << " " << observed << " " << expected << " " << count_left << "/" << total_left << " " << count_right << "/" << total_right << endl;
                return -1;
            }
            if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) diff = diff / 2.0;
            distance += 5.0 - diff;
        }
    }
    
    //cerr << "dist: " << distance << endl;
    
    return 100.0 * distance;
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
