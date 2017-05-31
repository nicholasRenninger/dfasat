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

#include "parameters.h"
//#include "state_merger.h"

using namespace std;

class apta;
class apta_node;
class APTA_iterator;
struct size_compare;
class evaluation_data;

typedef list<apta_node*> node_list;
typedef list<int> int_list;
typedef list<double> double_list;

typedef map<int, apta_node*> child_map;

typedef pair<bool, double> score_pair;
typedef pair< pair<int, int>, score_pair > ts_pair;
typedef map< apta_node*, ts_pair > score_map;
typedef list< pair< int, int > > size_list;

typedef set<apta_node*, size_compare> state_set;
typedef pair<apta_node*, apta_node*> merge_pair;
typedef list< merge_pair > merge_list;
typedef multimap<double, merge_pair > merge_map;

typedef list<apta_node*> node_list;
typedef list<int> int_list;
typedef list<double> double_list;

typedef map<int, apta_node*> child_map;
typedef map<int, int> num_map;

/** 
 * @brief Node structure in the prefix tree.
 *
 * The prefix tree is a pointer structure made
 * of apta_nodes.
 */
class apta_node{
public:

    apta* context; /**< pointer to the tree the node is part of (legacy) */
    apta_node* source; /**< parent state in the prefix tree */
    apta_node* representative; /**< UNION/FIND data structure for merges*/
    child_map children; /**< target states/children in the prefix tree*/
    child_map det_undo; /**< UNDO information */

    /** the incomming transition label */
    int label;
    
    /** the type of node (accepting/rejecting/other)*/
    int type;

    /** depth of the node in the apta */
    int depth;

    /** unique state identifiers, used by SAT encoding */
    int number;
    int satnumber;
    int colour;

    /** for streaming mode */ 
    int age;

    /** UNION/FIND size measure */
    int size;
    
    /** is this a red state? */
    bool red;
    
    /** store previously tested merges */
    score_map eval_store;
    size_list size_store;
    
    /** extra information for merging heursitics and consistency checks */
    evaluation_data* data;

    apta_node();
    apta_node(apta* context);
    ~apta_node();

    /* UNION/FIND */
    apta_node* find();
    apta_node* find_until(apta_node*, int);

    /* get transition target */
    apta_node* get_child(int);

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
};

/* iterators for the APTA and merged APTA */
class APTA_iterator {
public:
    apta_node* base;
    apta_node* current;
    
    APTA_iterator(apta_node* start);
    
    apta_node* next_forward();
    apta_node* next_backward();
    virtual void increment();
    
    apta_node* operator*() const { return current; }
    APTA_iterator& operator++() { increment(); return *this; }
};

class merged_APTA_iterator {
public:
    apta_node* base;
    apta_node* current;
    
    merged_APTA_iterator(apta_node* start);

    apta_node* next_forward();
    apta_node* next_backward();
    virtual void increment();
    
    apta_node* operator*() const { return current; }
    merged_APTA_iterator& operator++() { increment(); return *this; }
};

class blue_state_iterator : public merged_APTA_iterator {
public:
    
    blue_state_iterator(apta_node* start);

    void increment();    
};

class red_state_iterator : public merged_APTA_iterator {
public:
    
    red_state_iterator(apta_node* start);

    void increment();    
};

class merged_APTA_iterator_func : public merged_APTA_iterator {
public:
    
    bool(*check_function)(apta_node*);

    merged_APTA_iterator_func(apta_node* start, bool(*)(apta_node*));

    void increment();    
};

struct size_compare
{
    bool operator()(apta_node* left, apta_node* right) const
    {
        if(DEPTH_FIRST){
            if(left->depth > right->depth)
                return 1;
            if(left->depth < right->depth)
                return 0;
        } else {
            if(left->size > right->size)
                return 1;
            if(left->size < right->size)
                return 0;
        }
        return left->number < right->number;
    }
};

typedef set<apta_node*, size_compare> state_set;

#include "evaluate.h"

/** 
 * @brief Data structure for the  prefix tree.
 *
 * The prefix tree is a pointer structure made
 * of apta_nodes.
 * @see apta_node
 */

class apta{
public:
    state_merger *context;
    apta_node* root; /**< root of the tree */
    map<int, string> alphabet; /**< mapping between internal representation and alphabet symbol */
    int merge_count;
    int max_depth;
    
    apta();
    ~apta();

    string alph_str(int i); /**< return alphabet symbol for internal index i */
    void read_file(istream &input_stream);
    void print_dot(iostream& output);
    int sink_type(apta_node* apta);
};

#endif
