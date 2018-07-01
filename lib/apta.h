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

class apta;
class apta_node;
class APTA_iterator;
class child_iterator;
class all_child_iterator;
struct size_compare;
class evaluation_data;
class apta_guard;

#include "parameters.h"
#include "inputdata.h"
//#include "state_merger.h"

using namespace std;

typedef list<apta_node*> node_list;
typedef list<int> int_list;
typedef list<double> double_list;

//typedef map<int, apta_node*> child_map;

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

//typedef map<int, apta_node*> child_map;
typedef map<int, int> num_map;

typedef multimap<int, apta_guard*> guard_map;
typedef map<int, float> bound_map;

class apta_guard{
public:
    apta_node* target;
    apta_node* undo;
    
    bound_map min_attribute_values;
    bound_map max_attribute_values;
    
    apta_guard();
    apta_guard(apta_guard*);
    
    bool bounds_satisfy(tail* t);
};

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
    //child_map children; /**< target states/children in the prefix tree*/
    //child_map det_undo; /**< UNDO information */
    guard_map guards;
    tail* tails_head;
    void add_tail(tail* t);
    
    /** storing all states merged with this state */
    apta_node* next_merged_node;
    apta_node* representative_of;
    
    /** this gets merged with node, replacing head of list */
    inline void merge_with(apta_node* node){
        this->representative = node;
        this->next_merged_node = node->representative_of;
        node->representative_of = this;
        
        node->size += this->size;
        this->representative = node;
    };
    /** undo this gets merged with node, resetting head of list */
    inline void undo_merge_with(apta_node* node){
        this->representative = 0;
        node->representative_of = this->next_merged_node;
        this->next_merged_node = 0;
        
        node->size -= this->size;
        this->representative = 0;
    };

    /* FIND/UNION functions */
    inline apta_node* find(){
        apta_node* rep = this;
        while(rep->representative != 0) rep = rep->representative;
        return rep;
    };
    inline apta_node* find_until(apta_node* node, int i){
        apta_node* rep = this;
        while(rep->representative != 0 && rep->undo(i) != node) rep = rep->representative;
        if(rep->undo(i) == node) return this;
        return 0;
    };
    
    /* guards, children, and undo map access */
    apta_node* child(tail* t);
    apta_guard* guard(tail* t);

    inline apta_node* child(int i){
        guard_map::iterator it = guards.find(i);
        if(it != guards.end()) return (*it).second->target;
        return 0;
    };
    inline apta_guard* guard(int i){
        guard_map::iterator it = guards.find(i);
        if(it != guards.end()) return (*it).second;
        return 0;
    };
    inline void set_child(int i, apta_node* node){
        guard_map::iterator it = guards.find(i);
        if(it != guards.end()){
            (*it).second->target = node;
        } else{
            apta_guard* g = new apta_guard();
            guards.insert(pair<int,apta_guard*>(i,g));
            g->target = node;
        }
    };
    inline apta_node* undo(int i){
        guard_map::iterator it = guards.find(i);
        if(it != guards.end()) return (*it).second->undo;
        return 0;
    };
    inline void set_undo(int i, apta_node* node){
        guard_map::iterator it = guards.find(i);
        if(it != guards.end()){
            (*it).second->undo = node;
        } else{
            apta_guard* g = new apta_guard();
            guards.insert(pair<int,apta_guard*>(i,g));
            g->undo = node;
        }
    };
    inline apta_node* get_child(int c){
        apta_node* rep = find();
        if(rep->child(c) != 0) return rep->child(c)->find();
        return 0;
    };

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
};

/*
class child_iterator {
public:
    guard* current;
    
    child_iterator(guard* g);
    void increment_symbol();
    virtual void increment();
    
    apta_node* operator*() const { if(current != 0) return current->target; return 0; }
    child_iterator& operator++() { increment(); return *this; }
};

class all_child_iterator {
public:
    apta_node* node;
    child_iterator it1;
    guard_map::iterator it2;
    
    child_iterator(apta_node* n);
    void increment_symbol();
    virtual void increment();
    
    apta_node* operator*() const {
        if(it2 != node->guards.end()){
            return *it1;
        }
        return 0;
    }

    all_child_iterator& operator++() {
        it1++;
        if(it1 == 0){
            it2++;
            if(it2 != node->guards.end()){
                it1 = child_iterator((*it2).second);
            }
        }
        return *this;
    }
};
*/

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

class tail_iterator {
public:
    apta_node* base;
    apta_node* current;
    tail* current_tail;
    
    tail_iterator(apta_node* start);
    
    apta_node* next_forward();
    apta_node* next_backward();
    virtual void increment();
    
    tail* operator*() const { return current_tail; }
    tail_iterator& operator++() { increment(); return *this; }
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
    vector<string> alp; // taken from inputdata

    int merge_count;
    int max_depth;
    
    apta();
    ~apta();

    string alph_str(int i); /**< return alphabet symbol for internal index i */
    void read_file(istream &input_stream);
    void print_dot(iostream& output);
    void print_json(iostream& output);
    int sink_type(apta_node* apta);
};

#endif
