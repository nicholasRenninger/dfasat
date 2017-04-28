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
    score = LOWER_BOUND;
}

inline void refinement::print() const{
    cerr << "score " << score << endl;
};
	
inline void refinement::print_short() const{
    cerr << score;
};

inline void refinement::doref(state_merger* m){
};
	
inline void refinement::undo(state_merger* m){
};

inline void merge_refinement::print() const{
    cerr << "merge( " << left->number << " " << right->number << " )" << endl;
};
	
inline void merge_refinement::print_short() const{
    cerr << "m" << score;
};

inline void merge_refinement::doref(state_merger* m){
    m->perform_merge(left, right);
};
	
inline void merge_refinement::undo(state_merger* m){
    m->undo_perform_merge(left, right);
};

inline void extend_refinement::print() const{
    cerr << "extend( " << right->number << " )" << endl;
};
	
inline void extend_refinement::print_short() const{
    cerr << "x" << right->size;
};

inline void extend_refinement::doref(state_merger* m){
    m->extend(right);
};
	
inline void extend_refinement::undo(state_merger* m){
    m->undo_extend(right);
};
