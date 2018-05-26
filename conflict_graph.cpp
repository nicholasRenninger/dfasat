
#include "conflict_graph.h"

void apta_graph::add_conflicts(state_merger &merger){
	for(node_set::iterator it = nodes.begin(); it != nodes.end(); ++it){
		graph_node* left = *it;
		node_set::iterator next_it = it;
		next_it++;
		for(node_set::iterator it2 = next_it;
				it2 != nodes.end();
				++it2){
			graph_node* right = *it2;
			
			refinement* ref = merger.test_merge(left->anode, right->anode);

			if(ref != 0){
				left->neighbors.insert(right);
				right->neighbors.insert(left);
			}
		}
	}
}

apta_graph::apta_graph(state_set &states){
	for(state_set::iterator it = states.begin(); it != states.end(); ++it){
		nodes.insert(new graph_node(*it));
	}
}

graph_node::graph_node(apta_node* an){
	anode = an;
}

void apta_graph::remove_edges(int size){
	node_set to_remove;
	for(node_set::iterator it = nodes.begin(); it != nodes.end(); ++it){
		if((*it)->neighbors.size() < size)
			to_remove.insert(*it);
	}
	for(node_set::iterator it = to_remove.begin(); it != to_remove.end(); ++it){
		nodes.erase(*it);
		for(node_set::iterator it2 = (*it)->neighbors.begin(); it2 != (*it)->neighbors.end(); ++it2){
			(*it2)->neighbors.erase(*it);
		}
	}
}

node_set *apta_graph::find_clique_converge() {
	int size = 0;
	while(true){
		node_set* result = find_clique();
		if(result->size() > size){
			remove_edges( result->size() );
			size = result->size();
		} else {
			return result;
		}
	}
}

node_set *apta_graph::find_clique(){
	node_set* result = new node_set();

	if( nodes.empty() ) return result;

	graph_node* head = NULL;
	int max_degree = -1;
	for(node_set::iterator it = nodes.begin();
			it != nodes.end();
			++it){
		graph_node* n = *it;
		if((int)n->neighbors.size() > max_degree){
			max_degree = n->neighbors.size();
			head = n;
		}
	}

	result->insert(head);

	node_set intersection = (*head).neighbors;

	max_degree = -1;
	for(node_set::iterator it = nodes.begin();
			it != nodes.end();
			++it){
		graph_node* n = *it;
		if((int)n->neighbors.size() > max_degree){
			max_degree = n->neighbors.size();
			head = n;
		}
	}

	while(!intersection.empty()){
		result->insert(head);
		intersection.erase(head);
		node_set new_intersection = node_set(intersection);
		for(node_set::iterator it = intersection.begin();
				it != intersection.end();
				++it){
			graph_node* keep = *it;
			if(head->neighbors.find(keep) == head->neighbors.end()){
				new_intersection.erase(keep);
			}
		}
		intersection = new_intersection;

		int best_match = -1;
		for(node_set::iterator it = intersection.begin();
				it != intersection.end();
				++it){
			graph_node* current = *it;
			int match = 0;
			for(node_set::iterator it2 = intersection.begin();
					it2 != intersection.end();
					++it2){
				if(current->neighbors.find(*it2) != current->neighbors.end()){
					match++;
				}
			}
			if(match > best_match){
				best_match = match;
				head = current;
			}
		}
	}

	return result;
}

/*
 node_set *apta_graph::find_bipartite(){
	node_set* result = new node_set();

	if( nodes.empty() ) return result;

	graph_node* head = NULL;
	int max_degree = -1;
	for(node_set::iterator it = nodes.begin(); it != nodes.end(); ++it){
		graph_node* n = *it;
		if((int)n->neighbors.size() > max_degree){
			max_degree = n->neighbors.size();
			head = n;
		}
	}

	result->insert(head);

	node_set intersection = (*head).neighbors;

	max_degree = -1;
	for(node_set::iterator it = nodes.begin(); it != nodes.end(); ++it){
		graph_node* n = *it;
		if((int)n->neighbors.size() > max_degree){
			max_degree = n->neighbors.size();
			head = n;
		}
	}

	while(!intersection.empty()){
		result->insert(head);
		intersection.erase(head);
		node_set new_intersection = node_set(intersection);
		for(node_set::iterator it = intersection.begin(); it != intersection.end(); ++it){
			graph_node* keep = *it;
			if(head->neighbors.find(keep) == head->neighbors.end()){
				new_intersection.erase(keep);
			}
		}
		intersection = new_intersection;

		int best_match = -1;
		for(node_set::iterator it = intersection.begin();
				it != intersection.end();
				++it){
			graph_node* current = *it;
			int match = 0;
			for(node_set::iterator it2 = intersection.begin();
					it2 != intersection.end();
					++it2){
				if(current->neighbors.find(*it2) != current->neighbors.end()){
					match++;
				}
			}
			if(match > best_match){
				best_match = match;
				head = current;
			}
		}
	}

	return result;
}
 */

