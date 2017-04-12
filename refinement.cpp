#include <math.h>
#include <queue>
#include "refinement.h"
#include "parameters.h"

using namespace std;

merge_refinement::merge_refinement(double s, apta_node* l, apta_node* r){
    left = l;
    right = r;
    score = s;
}

extend_refinement::extend_refinement(apta_node* r){
    right = r;
}