
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

class state_merger;
class merger_context;

#include "apta.h"
#include "evaluate.h"

using namespace std;

class merger_context {
public:
    int literal_counter = 1;
    int clause_counter = 0;

    bool computing_header = true;
    
    state_merger* merger;
    state_set red_states;
    state_set non_red_states;
    state_set sink_states;

    FILE* sat_stream;

    int dfa_size;
    int sinks_size;
    int num_states;
    int new_states;
    int new_init;

    set<int> trueliterals;

    int best_solution = -1;

    void reset_literals(bool init);
    void create_literals();
    void delete_literals();
    int print_clause(bool v1, int l1, bool v2, int l2, bool v3, int l3, bool v4, int l4);
    int print_clause(bool v1, int l1, bool v2, int l2, bool v3, int l3);
    int print_clause(bool v1, int l1, bool v2, int l2);
    bool always_true(int number, bool flag);
    void print_lit(int number, bool flag);
    void print_clause_end();
    void fix_red_values();
    void fix_sink_values();
    int set_symmetry();
    int print_symmetry();
    void erase_red_conflict_colours();
    int print_colours();
    int print_conflicts();
    int print_accept();
    int print_transitions();
    int print_t_transitions();
    int print_p_transitions();
    int print_a_transitions();
    int print_forcing_transitions();
    int print_sink_transitions();
    int print_paths();
    int print_sink_paths();
    void print_dot_output(const char* dot_output);
    void print_aut_output(const char* aut_output);

};

//typedef set<apta_node*, total_weight_compare> state_set;
typedef list< pair<apta_node*, apta_node*> > merge_list;
typedef multimap<int, pair<apta_node*, apta_node*> > merge_map;
typedef pair<apta_node*, apta_node*> merge_pair;

typedef list<apta_node*> node_list;
typedef list<int> int_list;
typedef list<double> double_list;

typedef map<int, apta_node*> child_map;
typedef map<int, int> num_map;

class state_merger{
public:
    merger_context context;
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
    bool merge(apta_node* red, apta_node* blue);
    void merge_force(apta_node* red, apta_node* blue);
    bool merge_test(apta_node* red, apta_node* blue);
    void undo_merge(apta_node* red, apta_node* blue);

    /* find merges */
    merge_map &get_possible_merges();

    /* update the blue and red states */
    void update();

    /* find unmergable states */
    bool extend_red();

    int testmerge(apta_node*,apta_node*);
    int test_local_merge(apta_node* red, apta_node* blue);

    bool perform_merge(apta_node*, apta_node*);

    state_set &get_candidate_states();
    state_set &get_sink_states();

    int get_final_apta_size();

    void todot();
    void print_dot(FILE*);
    void read_apta(istream &input_stream);
    void read_apta(string dfa_file);
//    void read_apta(FILE* dfa_file);
//    void read_apta(boost::python::list dfa_data);
    void read_apta(vector<string> dfa_data);

    int sink_type(apta_node* node);
    bool sink_consistent(apta_node* node, int type);
    int num_sink_types();

    string dot_output;
};


#endif /* _STATE_MERGER_H_ */