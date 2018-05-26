
#ifndef _STATE_MERGER_H_
#define _STATE_MERGER_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <string>

class evaluation_data;
class evaluation_function;

using namespace std;

class apta;
class apta_node;
class guard;

struct size_compare;

extern int alphabet_size;
extern bool MERGE_SINKS_DSOLVE;

typedef set<apta_node*, size_compare> state_set;
typedef list<apta_node*> node_list;

typedef pair<apta_node*, apta_node*> merge_pair;
typedef list< pair<apta_node*, apta_node*> > merge_list;
typedef multimap<int, pair<apta_node*, apta_node*> > merge_map;

typedef list<guard*> guard_list;
typedef map<int, guard_list*> child_map;

class guard{
public:
    int type;
    int bound;
    int attr;
    
    apta_node* target;

    guard(int t, int b, int a);
    
    inline bool eval(vector<int>& attributes){
        switch(type){
            case 1:
                if(attributes[attr] == bound) return true;
            case 2:
                if(attributes[attr] < bound) return true;
            case 3:
                if(attributes[attr] >= bound) return true;
        }
        return false;
    };
};

class apta{
public:
    apta_node* root;
    int merge_count;
    
    apta(ifstream &input_stream);
    ~apta();
    
    state_set &get_states();
    state_set &get_accepting_states();
    state_set &get_rejecting_states();
};

class apta_node{
public:
    apta_node();
    ~apta_node();
    
    /* from input data */
    apta_node* prev_node;
    apta_node* next_node;

    /* UNION/FIND datastructure */
    apta_node* representative;
    child_map children;
    /* UNDO information */
    child_map det_undo;
    
    /* get transition targets */
    guard_list* get_targets(int);
    
    /* the incomming transition event label and its attributes */
    int label;
    vector<int> attributes;
    
    /* unique state identifiers, used by encoding */
    int number;
    int satnumber;
    int colour;
    
    /* UNION/FIND size measure */
    int size;
    
    /* data used for computing consistency and heuristics */
    evaluation_data edat;
    
    /* all nodes merged with this node */
	node_list merged_nodes;
    node_list::iterator merge_point;
    
    /* UNION/FIND */
    apta_node* find();
    apta_node* find_until(apta_node*, int);
    
    inline guard_list* child(int i){
        child_map::iterator it = children.find(i);
        if(it == children.end()) return 0;
        return (*it).second;
    }

    inline guard_list* undo(int i){
        child_map::iterator it = det_undo.find(i);
        if(it == det_undo.end()) return 0;
        return (*it).second;
    }
};

struct size_compare{
    bool operator()(apta_node* left, apta_node* right) const{
        if(left->size > right->size) return 1;
        if(left->size < right->size) return 0;
        return left < right;
    }
};

class state_merger{
public:
    apta* aut;
    /* core of merge targets */
    state_set red_states;
    /* fringe of merge candidates */
    state_set blue_states;
    
    evaluation_function* eval;
    
    state_merger();
    state_merger(evaluation_function*, apta*);
    
    void reset();
    
    /* state merging (recursive) */
    void merge(apta_node* red, apta_node* blue);
    void undo_merge(apta_node* red, apta_node* blue);
    
    /* find merges */
    merge_map &get_possible_merges();
    
    /* update the blue and red states */
    void update();
    
    /* find unmergable states */
    bool extend_red();
    
    int test_merge(apta_node*,apta_node*);
    
    bool perform_merge(apta_node*, apta_node*);
    
    state_set &get_candidate_states();
    state_set &get_sink_states();
    
    int get_final_apta_size();
    
    void todot(FILE*);
};

#endif /* _STATE_MERGER_H_ */
