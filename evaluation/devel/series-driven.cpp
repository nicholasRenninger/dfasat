#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "series-driven.h"

DerivedRegister<series_driven> series_driven::reg("series_driven");

/* Series driven, like overlap, but requires merges to be of the same depth */
bool series_driven::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(evaluation_function::consistent(merger,left,right) == false) return false;
  if(left->depth != right->depth){ inconsistency_found = true; return false; }
    
  int total_left = left->accepting_paths + left->rejecting_paths;
  int total_right = right->accepting_paths + right->rejecting_paths;
  
  //if(total_left < STATE_COUNT || total_right < STATE_COUNT){
    //if(left->source != 0 && right->source != 0 && left->source->find() != right->source->find())
    //    { inconsistency_found = true; return false; }
    //return true;
  //}

  //if(total_left >= STATE_COUNT && total_right >= STATE_COUNT){
    double bound = sqrt(1.0 / (double)(total_left))
                 + sqrt(1.0 / (double)(total_right));
    bound = bound * sqrt(0.5 * log(2.0 / CHECK_PARAMETER));

    for(int i = 0; i < alphabet_size; ++i){
        int count_left = left->pos(i) + left->neg(i);
        int count_right = right->pos(i) + right->neg(i);
        if(count_left >= SYMBOL_COUNT && count_right == 0){ inconsistency_found = true; return false; }
        if(count_right >= SYMBOL_COUNT && count_left == 0){ inconsistency_found = true; return false; }
        
        /*double gamma = 0.0;
          gamma = (double)(count_left) / (double)(total_left)
                - (double)(count_right) / (double)(total_right);
        if(gamma < 0) gamma = -gamma;
        //cerr << gamma << " " << count_left << "," << total_left << " " << count_right << "," << total_right << endl;
        if(total_left >= STATE_COUNT && total_right >= STATE_COUNT){
        if(count_left >= SYMBOL_COUNT || count_right >= SYMBOL_COUNT){
        if(gamma > bound){ inconsistency_found = true; return false; }}}
      //}*/
    }
  //}
  
    //return true;
    
    //return true;

    //if(left->occs.size() == 0 && right->occs.size() != 0){ inconsistency_found = true; return false; }
    //if(left->occs.size() != 0 && right->occs.size() == 0){ inconsistency_found = true; return false; }
    
    double left_size = 0.0;
    double right_size = 0.0;
    double mean_left = 0.0;
    double mean_right = 0.0;
    double mean_total = 0.0;
            
    apta_node* node = left;
    for(double_list::iterator it2 = node->occs.begin(); it2 != node->occs.end(); ++it2){
        mean_left = mean_left + (double)*it2;
    }
    left_size = left_size + node->occs.size();
    
    node = right;
    for(double_list::iterator it2 = node->occs.begin(); it2 != node->occs.end(); ++it2){
        mean_right = mean_right + (double)*it2;
    }
    right_size = right_size + node->occs.size();
    
    if(left_size == 0 || right_size == 0) return true;

    mean_total = (mean_left + mean_right) / (left_size + right_size);
    mean_right = mean_right / right_size;
    mean_left = mean_left / left_size;
            
    //if(left_size < STATE_COUNT || right_size < STATE_COUNT) return true;

    if (mean_left - mean_right > 2.5 or mean_right - mean_left > 2.5){
        inconsistency_found = true;
        return false;
    }

    return true;
    return true;
    
  return true;
  
        for(int i = 0; i < alphabet_size; ++i){
        if(merger->aut->alphabet[i][0] < 4) continue;
        if(left->pos(i) != 0 && right->pos(i) == 0){ inconsistency_found = true; return false; }
        if(left->pos(i) == 0 && right->pos(i) != 0){ inconsistency_found = true; return false; }
        if(left->neg(i) != 0 && right->neg(i) == 0){ inconsistency_found = true; return false; }
        if(left->neg(i) == 0 && right->neg(i) != 0){ inconsistency_found = true; return false; }
    }

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
        for(guard_map::iterator it = right->guards.begin(); it != right->guards.end(); ++it){
            score_right((*it).second->target->find(), depth + 1);
        }
    }
};

void series_driven::score_left(apta_node* left, int depth){
    if(left_dist[depth].find(left) != left_dist[depth].end()) return;
    left_dist[depth].insert(left);
    if(depth < left_dist.size() - 1){
        for(guard_map::iterator it = left->guards.begin(); it != left->guards.end(); ++it){
            score_left((*it).second->target->find(), depth + 1);
        }
    }
};

int series_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    if(left->depth != right->depth){ return -1; }
    //if(left->occs.size() == 0){ return -1; }
    //if(right->occs.size() == 0){ return -1; }
    //if(left->occs.size() == 0 && right->occs.size() != 0){ return -1; }
    //if(right->occs.size() == 0  && left->occs.size() != 0){ return -1; }

    /*    if(left->pos(i) == 0 && right->pos(i) != 0){ inconsistency_found = true; return false; }
        if(left->neg(i) != 0 && right->neg(i) == 0){ inconsistency_found = true; return false; }
        if(left->neg(i) == 0 && right->neg(i) != 0){ inconsistency_found = true; return false; }
    }*/


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
            //cerr << count_left << "," << total_left << " " << count_right << "," << total_right << endl;

            if(total_left == 0 || total_right == 0) continue;
            //if(total_left < STATE_COUNT || total_right < STATE_COUNT) continue;
            

/*  //if(total_left >= STATE_COUNT && total_right >= STATE_COUNT){
    double bound = sqrt(1.0 / (double)(total_left))
                 + sqrt(1.0 / (double)(total_right));
    bound = bound * sqrt(0.5 * log(2.0 / CHECK_PARAMETER));

    //for(int i = 0; i < alphabet_size; ++i){
      //if(count_left >= SYMBOL_COUNT || count_right >= SYMBOL_COUNT){
        double gamma = 0.0;
          gamma = (double)(count_left) / (double)(total_left)
                - (double)(count_right) / (double)(total_right);
        if(gamma < 0) gamma = -gamma;
        //cerr << gamma << " " << count_left << "," << total_left << " " << count_right << "," << total_right << endl;
        //if(total_left >= STATE_COUNT && total_right >= STATE_COUNT){
        //if(count_left >= SYMBOL_COUNT || count_right >= SYMBOL_COUNT){
        //if(gamma > bound){ inconsistency_found = true; return -1; }}}
        distance = distance + gamma;
      //}
    }
  }
  return distance;*/
  
            //if((double)count_left / (double)total_left - (double)count_right / (double)total_right)
            double observed = (double)count_left;
            double expected = (double)total_left * ((double)(count_left+count_right) / ((double)total_left + total_right));
            double diff = observed - expected;
            if(diff < 0.0) diff = - diff;

            //if(observed != 0) cerr << i << " " << a << " " << diff << " " << observed << " " << expected << " " << count_left << "/" << total_left << " " << count_right << "/" << total_right << endl;
            if (diff > 5){
                inconsistency_found = true;
                return -1;
            }
            //if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) diff = diff / 2.0;
            //distance += diff;

            observed = (double)count_right;
            expected = (double)total_right * ((double)(count_left+count_right) / ((double)total_left + total_right));
            diff = observed - expected;
            if(diff < 0.0) diff = - diff;
            if (diff > 5){
                inconsistency_found = true;
                return -1;
            }
            //if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) diff = diff / 2.0;
            distance += diff;
        }
    }
    //return distance;

    int symb_one = -1;
    int symb_two = -1;
    for(int i = 0; i < alphabet_size; ++i){
        if(merger->aut->alphabet[i][0] == 0) symb_one = i;
        if(merger->aut->alphabet[i][0] == 3) symb_two = i;
    }
    
    double mse = 0.0;
    
    int i;
    
            double left_size = 0.0;
            double right_size = 0.0;
            double mean_left = 0.0;
            double mean_right = 0.0;
            double mean_total = 0.0;
        
            int left_one = 0;
            int left_two = 0;
            int right_one = 0;
            int right_two = 0;
            
    for(i = 0; i < merger->aut->max_depth; ++i){
            for(state_set::iterator it = left_dist[i].begin(); it != left_dist[i].end(); ++it){
                apta_node* node = *it;
                for(double_list::iterator it2 = node->occs.begin(); it2 != node->occs.end(); ++it2){
                    mean_left = mean_left + (double)*it2;
                }
                left_size = left_size + node->occs.size();
                left_one += node->pos(symb_one) + node->neg(symb_one);
                left_two += node->pos(symb_two) + node->neg(symb_two);
            }
            
            for(state_set::iterator it = right_dist[i].begin(); it != right_dist[i].end(); ++it){
                apta_node* node = *it;
                for(double_list::iterator it2 = node->occs.begin(); it2 != node->occs.end(); ++it2){
                    mean_right = mean_right + (double)*it2;
                }
                right_size = right_size + node->occs.size();
                right_one += node->pos(symb_one) + node->neg(symb_one);
                right_two += node->pos(symb_two) + node->neg(symb_two);
            }
            
            //if(left_one != 0 && right_two != 0){ inconsistency_found = true; return false; }
            //if(left_two != 0 && right_one != 0){ inconsistency_found = true; return false; }
    }
    
            if(left_size == 0 || right_size == 0) return 0;

            mean_total = (mean_left + mean_right) / (left_size + right_size);
            mean_right = mean_right / right_size;
            mean_left = mean_left / left_size;
            
            //if(left_size < STATE_COUNT || right_size < STATE_COUNT) continue;
            mse = mse + ((mean_left - mean_right) * (mean_left - mean_right));

            //if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) continue;
            //continue;
        
            //if (mean_left - mean_right > 3 or mean_right - mean_left > 3){
            //    inconsistency_found = true;
            //    return -1;
            //}
    //}
    
    return mse;
    
    mse = mse / ((double)i+1);
    
    //cerr << "mse:" << mse << endl;
    //if(left->accepting_paths +  left->rejecting_paths < STATE_COUNT || right->accepting_paths +  right->rejecting_paths < STATE_COUNT)
    //    return -1;
    //if(left->source != 0 && right->source != 0 && left->source->find() == right->source->find()) mse = mse / 2.0;
    return mse;
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
