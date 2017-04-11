#include <math.h>
#include <queue>
#include "refinement.h"
#include "parameters.h"

using namespace std;

refinement::refinement(apta_node* l, apta_node* r){
    left = l;
    right = r;
}