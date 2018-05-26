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

split_refinement::split_refinement(double s, apta_node* r, tail* t, int a){
    split_point = t;
    right = r;
    score = s;
    attribute = a;
}

extend_refinement::extend_refinement(apta_node* r){
    right = r;
    score = LOWER_BOUND;
}

inline void refinement::print() const{
    cerr << "score " << score << endl;
};
	
inline void refinement::print_short() const{
    cout << score;
};

inline void refinement::doref(state_merger* m){
};
	
inline void refinement::undo(state_merger* m){
};

inline void merge_refinement::print() const{
    cerr << "merge( " << left->number << " " << right->number << " )" << endl;
};
	
inline void merge_refinement::print_short() const{
    cout << "m" << score;
};

inline void merge_refinement::doref(state_merger* m){
    m->perform_merge(left, right);
};
	
inline void merge_refinement::undo(state_merger* m){
    m->undo_perform_merge(left, right);
};

inline void split_refinement::print() const{
    cerr << "split( " << right->number << " " << inputdata::get_symbol(split_point) << " " <<attribute << " " << inputdata::get_value(split_point, attribute) << " )" << endl;
};
	
inline void split_refinement::print_short() const{
    cout << "s" << score;
};

inline void split_refinement::doref(state_merger* m){
    m->perform_split(right, split_point, attribute);
};
	
inline void split_refinement::undo(state_merger* m){
    m->undo_perform_split(right, split_point, attribute);
};

inline void extend_refinement::print() const{
    cerr << "extend( " << right->number << " )" << endl;
};
	
inline void extend_refinement::print_short() const{
    cout << "x" << right->size;
};

inline void extend_refinement::doref(state_merger* m){
    m->extend(right);
};
	
inline void extend_refinement::undo(state_merger* m){
    m->undo_extend(right);
};
