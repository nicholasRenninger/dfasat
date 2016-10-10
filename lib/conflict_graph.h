
#ifndef _CONFLICT_GRAPH_H_
#define _CONFLICT_GRAPH_H_

#include <set>
#include "state_merger.h"
#include "apta.h"

using namespace std;

class apta_graph;
class graph_node;

typedef set<graph_node*> node_set;

class graph_node{
public:
	node_set neighbors;
	apta_node* anode;
  
	graph_node(apta_node*);
};

struct neighbor_compare
{
    bool operator()(graph_node* left, graph_node* right) const
    {
        if(left->neighbors.size() > right->neighbors.size())
            return 1;
        if(left->neighbors.size() < right->neighbors.size())
            return 0;
        return left < right;
    }
};

typedef set<graph_node*, neighbor_compare> ordered_node_set;

class apta_graph{
public:
	ordered_node_set nodes;

	apta_graph(state_set &states);
  
	node_set* find_clique();
	node_set* find_clique_converge();
	void remove_edges(int);
	void add_conflicts(state_merger &merger);
};


#endif /* _CONFLICT_GRAPH_H_ */
