

#include <math.h>
#include <queue>
#include "searcher.h"
#include "parameters.h"

using namespace std;

bool SEARCH_DEEP = true;

/* queue used for searching */
struct refinement_list_compare{ bool operator()(const pair<double, refinement_list*> &a, const pair<double, refinement_list*> &b) const{ return a.first > b.first; } };
priority_queue< pair<double, refinement_list*>, vector< pair<double, refinement_list*> >, refinement_list_compare> Q;
refinement_list* current_refinements;

int greedy(state_merger* merger){
    int result = 0;
    
    //cerr << merger->compute_global_score() << endl;
    /* checked in state merger
    if(EXTEND_ANY_RED){
        apta_node* node = merger->extend_red();
        if(node != 0){
            //cerr << "+ ";
            result = greedy(merger);
            merger->undo_extend(node);
            return result;
        }
    }*/
    
    refinement* top_ref = merger->get_best_refinement();
    top_ref->doref(merger);
    result = greedy(merger);
    top_ref->undo(merger);
    delete top_ref;
    
	return result;
}

double compute_score(state_merger* merger){
    if(SEARCH_DEEP) return greedy(merger);
    return merger->get_final_apta_size();
    //refinement topref = merger->get_best_refinement();
    //return merger->test_merge(top_pair->first, top_pair->second).second;
}

void add_to_q(state_merger* merger){
    refinement_set* refs = merger->get_possible_refinements();

    if(refs->empty()){
        delete refs;
        return;
    }

	for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
        refinement* ref = *it;

        ref->doref(merger);
		double score = compute_score(merger);
        ref->undo(merger);

		refinement_list::iterator it2 = current_refinements->insert(current_refinements->end(), ref);
		Q.push(pair<double, refinement_list*>(score, new refinement_list(*current_refinements)));
		current_refinements->erase(it2);
	}
    delete refs;
}

void change_refinement_list(state_merger* merger, refinement_list* new_list){
	refinement_list::reverse_iterator old_it = current_refinements->rbegin();
	while(old_it != current_refinements->rend()){
		(*old_it)->undo(merger);
        delete *old_it;
		old_it++;
	}
	refinement_list::iterator new_it = new_list->begin();
	while(new_it != new_list->end()){
		(*new_it)->doref(merger);
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
		
        //cerr << endl;
        //cerr << "solution " << result << endl;
        
		if(best_solution == -1.0 || result < best_solution){
            cerr << "*** current best *** " << result << endl;
            best_solution = result;
        }
        
        add_to_q(merger);
	}
}
