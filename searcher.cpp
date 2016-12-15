/*
 *  RTI (real-time inference)
 *  Searcher.cpp, the source file for the search routines
 *  Currently, only a simple greedy (best-first) routine is implemented, search routines will be added later.
 *
 *  You can change greedy() to test() in order to have a nice testing environment for you data and the produced real-time automaton.
 *  Test allows you to make every decision of the algorithm yourself, given the p-values of the options.
 *
 *  The main routine is contained in this file
 *  It takes as arguments a TEST_TYPE (1 for likelihood ratio, 2 for chi squared), and the test SIGNIFiCANCE value
 *
 *  Some describtion of the algorithm:
 *  Sicco Verwer and Mathijs de Weerdt and Cees Witteveen (2007),
 *  An algorithm for learning real-time automata,
 *  In Maarten van Someren and Sophia Katrenko and Pieter Adriaans (Eds.),
 *  Proceedings of the Sixteenth Annual Machine Learning Conference of Belgium and the Netherlands (Benelearn),
 *  pp. 128-135.
 *  
 *  Copyright 2009 - Sicco Verwer, jan-2009
 *  This program is released under the GNU General Public License
 *  Info online: http://www.gnu.org/licenses/quick-guide-gplv3.html
 *  Or in the file: licence.txt
 *  For information/questions contact: siccoverwer@gmail.com
 *
 *  I will try to keep this software updated and will also try to add new statistics or search methods.
 *  Also I will add comments to the source in the near future.
 *
 *  Feel free to adapt the code to your needs, please inform me of (potential) improvements.
 */

#include <gsl/gsl_cdf.h>
#include <math.h>
#include <queue>
#include "searcher.h"

using namespace std;

/* queue used for searching */
struct refinement_list_compare{ bool operator()(const pair<double, refinement_list*> &a, const pair<double, refinement_list*> &b) const{ return a.first < b.first; } };
priority_queue< pair<double, refinement_list*>, vector< pair<double, refinement_list*> >, refinement_list_compare> Q;
refinement_list* current_refinements;
double best_solution = -1;

refinement::refinement(apta_node* left, apta_node* right){
    left = l;
    right = r;
}

int greedy(state_merger* merger){
    int result = 0;
    
    merge_map* possible_merges = merger->get_possible_merges();
    if(!possible_merges->empty()){
        merge_pair top_pair = (*possible_merges.rbegin()).second;
        //float top_score = (*possible_merges.rbegin()).first;
        merger->perform_merge(top_pair.first, top_pair.second);
        result = greedy(merger);
        merger->undo_merge(top_pair.first, top_pair.second);
    } else if (merger->extend_red() == true){
        result = greedy(merger);
    } else {
        result = merger->get_final_apta_size();
    }
    delete possible_merges;
    
    merger.reset();
	return result;
}

void add_merges_to_q(refinement_set &refinements){
	for(refinement_set::iterator it = refinements.begin(); it != refinements.end(); ++it){
		TA->check_consistency();
		
		(*it).second.refine();
		double score = greedy();
		(*it).second.undo_refine();

		refinement_list::iterator it2 = current_refinements->insert(current_refinements->end(), (*it).second);
		
		Q.push(pair<double, refinement_list*>(score, new refinement_list(*current_refinements)));
		current_refinements->erase(it2);
	}
}

void change_refinement_list(refinement_list* new_list){
	refinement_list::reverse_iterator old_it = current_refinements->rbegin();
	while(old_it != current_refinements->rend()){
		(*old_it).undo_refine();
		old_it++;
	}
	refinement_list::iterator new_it = new_list->begin();
	while(new_it != new_list->end()){
		(*new_it).refine();
		new_it++;
	}

	delete current_refinements;
	current_refinements = new_list;
};


void bestfirst(){
	current_refinements = new refinement_list();
	pair<refinement_set*,refinement_set*> refinements = get_best_refinements();

	refinement_set new_refinements;
	int num_points = 0;
	int num_splits = 0;
	for(refinement_set::reverse_iterator it = refinements.second->rbegin(); it != refinements.second->rend(); ++it){
		if((*it).first < SIGNIFICANCE) new_refinements.insert(*it);
		num_splits++;
		if(num_splits == max_splits_to_search) break;
	}
	if(new_refinements.empty()){
		for(refinement_set::iterator it = refinements.first->begin(); it != refinements.first->end(); ++it){
			if((*it).first >= SIGNIFICANCE) new_refinements.insert(*it);
			num_points++;
			if(num_points == max_points_to_search) break;
		}
	}
	add_merges_to_q(new_refinements);
	
	while(!Q.empty()){
		NODES++;
		
		pair<double, refinement_list*> next_refinements = Q.top();
		change_refinement_list(next_refinements.second);
		Q.pop();
		
		double aic = calculate_aic_without_default();
		
		if(best_solution != -1.0 && aic > best_solution) continue;

		refinements = get_best_refinements();
		new_refinements = refinement_set();
		num_points = 0;
		num_splits = 0;
		for(refinement_set::reverse_iterator it = refinements.second->rbegin(); it != refinements.second->rend(); ++it){
			if((*it).first < SIGNIFICANCE) new_refinements.insert(*it);
			num_splits++;
			if(num_splits == max_splits_to_search) break;
		}
		if(new_refinements.empty()){
			for(refinement_set::iterator it = refinements.first->begin(); it != refinements.first->end(); ++it){
				if((*it).first >= SIGNIFICANCE) new_refinements.insert(*it);
				num_points++;
				if(num_points == max_points_to_search) break;
			}
		}
		
		if(new_refinements.empty()){
			continue;
		} else {
			add_merges_to_q(new_refinements);
		}
		
		delete refinements.first;
		delete refinements.second;
	}
}

void test(){
	NODES++;
	
	pair<refinement_set*,refinement_set*> refinements = get_best_refinements();

	if(refinements.first->empty() && refinements.second->empty()){
		cout << TA->to_str();
		cout.flush();
		delete refinements.first;
		delete refinements.second;
		return;
	}

	TA->check_consistency();

 	cerr << TA->to_str();
 	cerr << "\nOPTIONS:\n";
	int i = 0;
	for(refinement_set::reverse_iterator it = refinements.first->rbegin(); it != refinements.first->rend(); ++it){
		cerr << i++ << ": ";
		(*it).second.print();
		cerr << " score: " << (*it).first << endl;
	}
	int spl = 0;
	for(refinement_set::reverse_iterator it = refinements.second->rbegin(); it != refinements.second->rend(); ++it){
		cerr << i++ << ": ";
		(*it).second.print();
		cerr << " score: " << (*it).first << endl;
		
		if(++spl == 20) break;
	}
	cerr << endl;
	cout << "choose a number." << endl;
	int number;
	cin >> number;

	i = 0;
	for(refinement_set::iterator it = refinements.first->begin(); it != refinements.first->end(); ++it){
		if(i++ != number)
			continue;
		(*it).second.refine();
		test();
	}
	for(refinement_set::iterator it = refinements.second->begin(); it != refinements.second->end(); ++it){
		if(i++ != number)
			continue;
		(*it).second.refine();
		test();
	}

	delete refinements.first;
	delete refinements.second;
}

int main(int argc, const char *argv[]){
	if(argc != 4){
		cerr << "Usage: ./rti TEST_TYPE SIGNIFICANCE file" << endl;
		cerr << "  TEST_TYPE is 1 for likelihood ratio, 2 for chi squared" << endl;
		cerr << "  SIGNIFICANCE is a decision (float) value between 0.0 and 1.0, default is 0.05 (5% significance)" << endl;
		cerr << "  file is an input file conaining unlabeled timed strings" << endl;
		return 0;
	}
	
	ifstream test_file(argv[3]);
	if(!test_file.is_open())
		return 0;
	
	timed_input *in = new timed_input(test_file);
	test_file.close();
	
	TEST_TYPE = atoi(argv[1]);
	SIGNIFICANCE = atof(argv[2]);
	
	TA = new timed_automaton(in);	
	bestfirst();
	
	return 1;
}
