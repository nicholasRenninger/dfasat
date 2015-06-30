
#ifndef _STATE_MERGER_H_
#define _STATE_MERGER_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include <map>

class evaluation_data;
class evaluation_function;

using namespace std;

class apta;
class apta_node;

class merge_compare;
struct size_compare;
struct total_weight_compare;
struct positive_weight_compare;

extern int alphabet_size;
extern bool MERGE_SINKS_DSOLVE;

typedef set<apta_node*, total_weight_compare> state_set;
typedef list< pair<apta_node*, apta_node*> > merge_list;
typedef multimap<int, pair<apta_node*, apta_node*> > merge_map;
typedef pair<apta_node*, apta_node*> merge_pair;
typedef list<apta_node*> node_list;

bool is_accepting_sink(apta_node* node);
bool is_rejecting_sink(apta_node* node);

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
    /* UNION/FIND datastructure */
    apta_node* source;
    apta_node* representative;
    vector<apta_node*> children;
    /* UNDO information */
    vector<apta_node*> det_undo;
    
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
    vector<int> num_pos;
    vector<int> num_neg;
    
    /* depth of the node in the apta */
    int depth;
    int old_depth;
    
    /* counts of positive and negative endings */
    int num_accepting;
    int num_rejecting;
    
    /* counts of positive and negative traversals */
    int accepting_paths;
    int rejecting_paths;
    
    apta_node();
    ~apta_node();
    
    /* UNION/FIND */
    apta_node* find();
    apta_node* find_until(apta_node*, int);
};

struct size_compare
{
    bool operator()(apta_node* left, apta_node* right) const
    {
        if(left->size > right->size)
            return 1;
        if(left->size < right->size)
            return 0;
        return left < right;
    }
};

struct positive_weight_compare
{
    bool operator()(apta_node* left, apta_node* right) const
    {
        int left_weight = left->num_accepting + left->accepting_paths;
        int right_weight = right->num_accepting + right->accepting_paths;
        
        if(left_weight > right_weight)
            return 1;
        if(left_weight < right_weight)
            return 0;
        return left < right;
    }
};

struct total_weight_compare
{
    bool operator()(apta_node* left, apta_node* right) const
    {
        int left_weight = left->accepting_paths + left->rejecting_paths;
        int right_weight = right->accepting_paths + right->rejecting_paths;
        
        if(left_weight > right_weight)
            return 1;
        if(left_weight < right_weight)
            return 0;
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
    
    int testmerge(apta_node*,apta_node*);
    
    bool perform_merge(apta_node*, apta_node*);
    
    state_set &get_candidate_states();
    state_set &get_sink_states();
    
    int get_final_apta_size();
    
    void todot(FILE*);
};

#endif /* _STATE_MERGER_H_ */
