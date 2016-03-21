#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "mealy.h"

//DerivedRegister<series_driven> series_driven::reg("series-driven");
REGISTER_DEF_TYPE(depth_driven);
/* RPNI like, merges shallow states (of lowest depth) first */

bool mealy::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(overlap_driven::consistent(merger,left,right) == false) return false;
    
    for(input_map::iterator it = left->input_output.begin(); it != left->input_output.end()){
        input_map::iterator it2 = right->input_output.find(it->first);
        if(it2 != right->input_map.end() && it->second != it2->second){
            inconsistency_found = true;
            return false;
        }
    }
    return true;
};
