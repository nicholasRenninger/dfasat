
#include <math.h>
#include <queue>
#include "searcher.h"
#include "parameters.h"

using namespace std;

/* queue used for searching */
struct refinement_list_compare{ bool operator()(const pair<double, refinement_list*> &a, const pair<double, refinement_list*> &b) const{ return a.first > b.first; } };
priority_queue< pair<double, refinement_list*>, vector< pair<double, refinement_list*> >, refinement_list_compare> Q;
refinement_list* current_refinements;

refinement::refinement(apta_node* l, apta_node* r){
    left = l;
    right = r;
}

int greedy(state_merger* merger){
    int result = 0;
    
    //cerr << merger->compute_global_score() << endl;
    if(EXTEND_ANY_RED){
        apta_node* node = merger->extend_red();
        if(node != 0){
            cerr << "+ ";
            result = greedy(merger);
            merger->undo_extend(node);
            return result;
        }
    }
    merge_pair* top_pair = merger->get_best_merge();
    if(top_pair->first != 0){
        cerr << merger->testmerge(top_pair->first, top_pair->second) << " ";
        merger->perform_merge(top_pair->first, top_pair->second);
        result = greedy(merger);
        merger->undo_perform_merge(top_pair->first, top_pair->second);
    } else {
        apta_node* node = merger->extend_red();
        if(node != 0){
            cerr << "+ ";
            result = greedy(merger);
            merger->undo_extend(node);
        } else {
            result = merger->compute_global_score();
        }
    }
    delete top_pair;
    
	return result;
}

void add_to_q(state_merger* merger){
    merge_map* possible_merges = merger->get_possible_merges();

    if(possible_merges->empty()){
        apta_node* node = merger->extend_red();
        if(node != 0){
            refinement ref = refinement(0, node);
            double score = greedy(merger);
            refinement_list::iterator it2 = current_refinements->insert(current_refinements->end(), ref);
            Q.push(pair<double, refinement_list*>(score, new refinement_list(*current_refinements)));
            current_refinements->erase(it2);
            merger->undo_extend(node);
        }
    }
    
	for(merge_map::iterator it = possible_merges->begin(); it != possible_merges->end(); ++it){
        refinement ref = refinement((*it).second.first, (*it).second.second);

        ref.doref(merger);
		double score = greedy(merger);
        ref.undo(merger);

		refinement_list::iterator it2 = current_refinements->insert(current_refinements->end(), ref);
		Q.push(pair<double, refinement_list*>(score, new refinement_list(*current_refinements)));
		current_refinements->erase(it2);
	}
    delete possible_merges;
}

void change_refinement_list(state_merger* merger, refinement_list* new_list){
	refinement_list::reverse_iterator old_it = current_refinements->rbegin();
	while(old_it != current_refinements->rend()){
		(*old_it).undo(merger);
		old_it++;
	}
	refinement_list::iterator new_it = new_list->begin();
	while(new_it != new_list->end()){
		(*new_it).doref(merger);
		new_it++;
	}

	delete current_refinements;
	current_refinements = new_list;
}

void bestfirst(state_merger* merger){
	merger->reset();
    int best_solution = -1;
    current_refinements = new refinement_list();
	add_to_q(merger);
    
    cerr << Q.size() << endl;
	
	while(!Q.empty()){
		pair<double, refinement_list*> next_refinements = Q.top();
		change_refinement_list(merger, next_refinements.second);
		Q.pop();
		
		double result = next_refinements.first;
		
        cerr << endl;
        cerr << "solution " << result << endl;
        
		if(best_solution == -1.0 || result < best_solution){
            cerr << "*** current best *** " << result << endl;
            best_solution = result;
        }
        
        add_to_q(merger);
	}
}
