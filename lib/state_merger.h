
#ifndef _STATE_MERGER_H_
#define _STATE_MERGER_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_map>
#include <string>

#include "apta.h"

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

//typedef set<apta_node*, total_weight_compare> state_set;
typedef list< pair<apta_node*, apta_node*> > merge_list;
typedef multimap<int, pair<apta_node*, apta_node*> > merge_map;
typedef pair<apta_node*, apta_node*> merge_pair;
typedef list<apta_node*> node_list;
typedef list<int> int_list;
typedef list<double> double_list;

typedef map<int, apta_node*> child_map;
typedef map<int, int> num_map;


bool is_accepting_sink(apta_node* node);
bool is_rejecting_sink(apta_node* node);

class state_merger{
public:
    apta* aut;
    /* core of merge targets */
    state_set red_states;
    /* fringe of merge candidates */
    state_set blue_states;

    evaluation_function* eval;

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
    void read_apta(FILE*);

    int sink_type(apta_node* node);
    bool sink_consistent(apta_node* node, int type);
    int num_sink_types();
};

#endif /* _STATE_MERGER_H_ */
