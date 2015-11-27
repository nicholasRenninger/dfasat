#ifndef __APTA_H__
#define __APTA_H__


#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_map>
#include <string>

using namespace std;

extern int alphabet_size;
extern bool MERGE_SINKS_DSOLVE;

class apta;
class apta_node;

class merge_compare;
struct size_compare;
struct total_weight_compare;
struct positive_weight_compare;

typedef set<apta_node*, total_weight_compare> state_set;
typedef list< pair<apta_node*, apta_node*> > merge_list;
typedef multimap<int, pair<apta_node*, apta_node*> > merge_map;
typedef pair<apta_node*, apta_node*> merge_pair;
typedef list<apta_node*> node_list;
typedef list<int> int_list;
typedef list<double> double_list;

typedef map<int, apta_node*> child_map;
typedef map<int, int> num_map;

class apta_node{
public:
    /* UNION/FIND datastructure */
    apta_node* source;
    apta_node* representative;
    child_map children;
    /* UNDO information */
    child_map det_undo;

    /* get transition target */
    apta_node* get_child(int);

    /* the incomming transition label */
    int label;

    /* unique state identifiers, used by encoding */
    int number;
    int satnumber;
    int colour;

    /* UNION/FIND size measure */
    int size;

    /* counts of positive and negative transition uses */
    num_map num_pos;
    num_map num_neg;

    /* depth of the node in the apta */
    int depth;
    int old_depth;

    /* counts of positive and negative endings */
    int num_accepting;
    int num_rejecting;

    /* counts of positive and negative traversals */
    int accepting_paths;
    int rejecting_paths;

	double_list occs;
    double_list::iterator occ_merge_point;

	node_list conflicts;
    node_list::iterator merge_point;

    apta_node();
    ~apta_node();

    /* UNION/FIND */
    apta_node* find();
    apta_node* find_until(apta_node*, int);

    inline apta_node* child(int i){
        child_map::iterator it = children.find(i);
        if(it == children.end()) return 0;
        return (*it).second;
    }

    inline apta_node* undo(int i){
        child_map::iterator it = det_undo.find(i);
        if(it == det_undo.end()) return 0;
        return (*it).second;
    }

    inline int pos(int i){
        num_map::iterator it = num_pos.find(i);
        if(it == num_pos.end()) return 0;
        return (*it).second;
    }

    inline int neg(int i){
        num_map::iterator it = num_neg.find(i);
        if(it == num_neg.end()) return 0;
        return (*it).second;
    }
};



class apta{
public:
    apta_node* root;
    map<int, vector<int> > alphabet;
    int merge_count;
    int max_depth;

    apta(ifstream &input_stream);
    ~apta();

    state_set &get_states();
    state_set &get_accepting_states();
    state_set &get_rejecting_states();

    string alph_str(int i);
};



#endif
