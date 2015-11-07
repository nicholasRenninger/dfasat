#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

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

