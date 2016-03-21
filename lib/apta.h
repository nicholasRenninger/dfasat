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

class evaluation_data;

using namespace std;

extern int alphabet_size;
extern bool MERGE_SINKS_DSOLVE;

class apta;
class apta_node;

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
    
    /* extra information for merging heursitics and consistency checks */
    evaluation_data data;

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
};

class apta{
public:
    apta_node* root;
    map<int, string> alphabet;
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
