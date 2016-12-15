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

int NODES = 0;
double SIGNIFICANCE = 0.05;

/* queue used for searching */
struct refinement_list_compare{ bool operator()(const pair<double, refinement_list*> &a, const pair<double, refinement_list*> &b) const{ return a.first < b.first; } };
priority_queue< pair<double, refinement_list*>, vector< pair<double, refinement_list*> >, refinement_list_compare> Q;
refinement_list* current_refinements;
double best_solution = -1;
int max_points_to_search = 10;
int max_splits_to_search = 10;

int calculate_parameters(){
	return ((NUM_HISTOGRAM_BARS - 1) * TA->num_states()) + TA->get_size();
};

/* AIC per timed symbol in the input data, giving timed symbols that are not parsed the default probability */
double calculate_aic(){
	double result = 0.0;
	double num_tests = 0;
	double default_log = log(1.0 / ((double)(NUM_HISTOGRAM_BARS + MAX_SYMBOL)));

	for(int i = 0; i < TA->num_states(); ++i){
		timed_state* st = TA->get_state(i);
		for(int s = 0; s < MAX_SYMBOL; ++s){
			double symbol_prob = ((double)st->stat->get_symbol_counts(s)) / ((double)st->stat->get_total_counts());
			if(symbol_prob != 0){
				double log_prob = log(symbol_prob);
				result += log_prob * (double)st->stat->get_symbol_counts(s);
				num_tests += ((double)st->stat->get_symbol_counts(s)) / 2.0;
			}
			for(const_interval_it it2 = st->get_intervals(s).begin(); it2 != st->get_intervals(s).end(); ++it2){
				interval* inter = (*it2).second;
				if(TA->contains_state(inter->get_target()) || inter->is_empty()) continue;
			 
				for(const_tail_it it3 = inter->get_tails().begin(); it3 != inter->get_tails().end(); ++it3){
					timed_tail* tail = (*it3).second;
			 
					if(tail->next_tail() != 0){
						result += default_log * (tail->get_length() - 1);
						num_tests += (double)(tail->get_length() - 1);
					}
				}
			 }
		}
		for(int t = 0; t < NUM_HISTOGRAM_BARS; ++t){
			double time_prob = ((double)st->stat->get_time_counts(t)) / ((double)st->stat->get_total_counts());
			if(time_prob != 0){
				double log_prob = log(time_prob);
				result += log_prob * (double)st->stat->get_time_counts(t);
				num_tests += ((double)st->stat->get_symbol_counts(t)) / 2.0;
			}
		}
	}
	return (2.0 * ((double)calculate_parameters())) - (2.0 * result);
}

double calculate_aic_without_default(){
	double result = 0.0;
	double num_tests = 0;
	
	for(int i = 0; i < TA->num_states(); ++i){
		timed_state* st = TA->get_state(i);
		for(int s = 0; s < MAX_SYMBOL; ++s){
			double symbol_prob = ((double)st->stat->get_symbol_counts(s)) / ((double)st->stat->get_total_counts());
			if(symbol_prob != 0){
				double log_prob = log(symbol_prob);
				result += log_prob * (double)st->stat->get_symbol_counts(s);
				num_tests += ((double)st->stat->get_symbol_counts(s)) / 2.0;
			}
		}
		for(int t = 0; t < NUM_HISTOGRAM_BARS; ++t){
			double time_prob = ((double)st->stat->get_time_counts(t)) / ((double)st->stat->get_total_counts());
			if(time_prob != 0){
				double log_prob = log(time_prob);
				result += log_prob * (double)st->stat->get_time_counts(t);
				num_tests += ((double)st->stat->get_symbol_counts(t)) / 2.0;
			}
		}
	}
	return (2.0 * ((double)calculate_parameters())) - (2.0 * result);
}

refinement::refinement(int s, int t, int sy, int ti){
	state = s;
	target = t;
	symbol = sy;
	time = ti;
	
	ref_count = 0;
}

pair<refinement_set*,refinement_set*> get_best_refinements(){
	TA->check_consistency();
	pair<refinement_set*,refinement_set*> result;

	refinement_set *merges = new refinement_set();
	refinement_set *splits = new refinement_set();
	
	result.first = merges;
	result.second = splits;
	
	interval* in = 0;
	int state = 0;
	int num_intervals = -1;
	int symbol = -1;
	int max_size = -1;
	
	for(int i = 0; i < TA->num_states(); ++i){
		timed_state* st = TA->get_state(i);
		for(int s = 0; s < MAX_SYMBOL; ++s){
			for(const_interval_it it2 = st->get_intervals(s).begin(); it2 != st->get_intervals(s).end(); ++it2){
				interval* inter = (*it2).second;
				if(TA->contains_state(inter->get_target()) || inter->is_empty()) continue;

				if(max_size == -1 || inter->get_tails().size() > max_size){
					in = inter;
					num_intervals = st->get_intervals(s).size();
					state = i;
					symbol = s;
					max_size = in->get_tails().size();
				}
			}
		}
	}
	
	if(in == 0) return result;
	if(in->get_tails().size() < 2 * MIN_DATA) return result;

	TA->check_consistency();
	
	for(int i = 0; i < TA->num_states(); ++i){
		timed_state* target = TA->get_state(i);
		if(target != TA->get_root()){
			double score = TA->get_state(state)->test_point(symbol, in->get_end(), target);
			if(score != -1.0) merges->insert(pair<double, refinement>(score, refinement(state, i, symbol, in->get_end())));
		}
	}
	TA->check_consistency();

	merges->insert(pair<double, refinement>(SIGNIFICANCE, refinement(state, -2, symbol, in->get_end())));
	
	int time = (*in->get_tails().begin()).first;
	for(const_tail_it it3 = in->get_tails().begin(); it3 != in->get_tails().end(); ++it3){
		timed_tail* tail = (*it3).second;
		if(time < tail->get_time_value()){
			double score = TA->get_state(state)->test_split(symbol, time);
			if(score != -1.0) splits->insert(pair<double, refinement>(score, refinement(state, -1, symbol, time)));
			time = (*it3).first;
		}
	}
	TA->get_state(state)->clear_marked(in);
	
	TA->check_consistency();
	return result;
}

int greedy(){
	NODES++;
	
	pair<refinement_set*,refinement_set*> refinements = get_best_refinements();
	
	if(refinements.first->empty() && refinements.second->empty()){
		double aic = calculate_aic();
		if(best_solution == -1.0 || aic < best_solution){
			cout << "SOLUTION:\n" << TA->to_str();
			cout << "SCORE = " << aic << endl;
			best_solution = aic;
		}
		delete refinements.first;
		delete refinements.second;
		
		return aic;
	}
	
	TA->check_consistency();
	
    if(!refinements.second->empty()){
		if((*refinements.second->rbegin()).first < SIGNIFICANCE)
			(*refinements.second->rbegin()).second.refine();
		else (*refinements.first->begin()).second.refine();
	}
	else (*refinements.first->begin()).second.refine();
	
	TA->check_consistency();
	
	int result = greedy();
	
    if(!refinements.second->empty()){
		if((*refinements.second->rbegin()).first < SIGNIFICANCE)
			(*refinements.second->rbegin()).second.undo_refine();
		else (*refinements.first->begin()).second.undo_refine();
	}
	else (*refinements.first->begin()).second.undo_refine();

	delete refinements.first;
	delete refinements.second;
	
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
