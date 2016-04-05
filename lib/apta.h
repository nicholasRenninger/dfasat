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

class apta;
class apta_node;
class evaluation_data;
struct size_compare;

typedef list<apta_node*> node_list;
typedef list<int> int_list;
typedef list<double> double_list;

typedef map<int, apta_node*> child_map;
typedef set<apta_node*, size_compare> state_set;

class apta_node{
public:
    /* parent state in the prefix tree */
    apta_node* source;
    /* UNION/FIND datastructure */
    apta_node* representative;
    /* target states */
    child_map children;
    /* UNDO information */
    child_map det_undo;

    /* the incomming transition label */
    int label;
    
    /* the type of node (accepting/rejecting/other)*/
    int type;

    /* depth of the node in the apta */
    int depth;

    /* unique state identifiers, used by SAT encoding */
    int number;
    int satnumber;
    int colour;

    /* UNION/FIND size measure */
    int size;
    
    /* extra information for merging heursitics and consistency checks */
    evaluation_data* data;

    apta_node();
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

class apta{
public:
    apta_node* root;
    map<int, string> alphabet;
    int merge_count;
    int max_depth;

    apta();
    ~apta();

    state_set &get_states();
    state_set &get_accepting_states();
    state_set &get_rejecting_states();

    string alph_str(int i);
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

#endif
