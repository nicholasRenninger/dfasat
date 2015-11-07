#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

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


