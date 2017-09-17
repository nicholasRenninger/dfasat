/// @file interactive.cpp
/// @brief All the functions and definitions for interactive merge mode.
/// @author Christian Hammerschmidt, hammerschmidt@posteo.de

#include <stdio.h>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <sstream>

#include "refinement.h"
#include "random_greedy.h"
#include "parameters.h"

/*! @brief Main loop for interactive mode.
 *         
 *  Constructs batch APTA, gets and prints possible merges, prompts user for interaction, executes command.
 *  Loops until some terminal condition is reached or user terminates the session.
 *  Outputs into two dot files to visualize current step/last step (merged) APTA.
 * @param[in] merger state_merger* object
 * @param[in] param  parameters* object to set global variables
 * @return refinement_list* list of refinments executed by the state merger
 */
refinement_list* interactive(state_merger* merger, parameters* param){
    cerr << "starting greedy merging" << endl;
    int num = 1;
    refinement_list* all_refs = new refinement_list();
    merger->eval->initialize(merger);

    string command = string("");
    string arg;
    int choice = 1; // what merge was picked
    int pos = 0; // what merge we are looking at
    int step = 1; // current step number
    int countdown = 0; // where to get to

    // if there is only one choice, do we auto-execute
    // or read for confirmation & chance for parameter changes?
    bool manual = false;
    bool execute = false;

    refinement_set* refs;
    refinement* chosen_ref;

    while( true ){
        merger->reset();
        while( true ){
            cout << " ";

          while(!execute) {

	    // output current merged apta
            merger->todot();
            std::ostringstream oss2;
            oss2 << param->dot_file <<"pre_" << num % 2 << ".dot";
            ofstream output(oss2.str().c_str());
            output << merger->dot_output;
            output.close();
           
            refs = merger->get_possible_refinements();
            chosen_ref = *refs->begin();

            cerr << endl;

            // if there is more than one option
	    if(refs->size() > 1 && !manual) {

              cout << "Possible refinements: " << endl;
              for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
                  (*it)->print_short();
                  cout << " , ";
              }

              cout << endl << "Your choice at step " << step << ": " << endl;
              getline(std::cin, command);
	      stringstream cline(command);
              cline >> arg;

	      // parse (prefix-free) commands 
	      if(arg  == "undo") {
                // undo single merge
                (*all_refs->begin())->undo(merger);
                all_refs->pop_front();
                step--;
                cout << "undid last merge" << endl;
	      } else if(arg == "restart") {
                // undo all merges up to here
                while(all_refs->begin() != all_refs->end()) {
                  (*all_refs->begin())->undo(merger);
                  all_refs->pop_front();
                }
                step = 1;
                cout << "Restarted" << endl;
              } else if (arg == "leap") {
                // automatically do the next n steps
                // TODO: error handling for n = NaN 
 	 	cline >> arg;
                countdown = stoi(arg);
               
                manual = true;
                arg = string("1");
                execute = true;
                break;
              } else if(arg == "set") {
	        // set PARAM <value>, call init_with_param

		cline >> arg;

		if(arg == "state_count") {
		  cline >> arg;
                  param->state_count = stoi(arg);
		  init_with_params(param);
		  cout << "STATE_COUNT is now " << STATE_COUNT << endl;
                }
 		if(arg == "symbol_count") {
		  cline >> arg;
                  param->symbol_count = stoi(arg);
		  init_with_params(param);
		  cout << "SYMBOL_COUNT is now " << SYMBOL_COUNT << endl;
                }           
		if(arg == "lower_bound") {
		  cline >> arg;
                  param->lower_bound = stoi(arg);
		  init_with_params(param);
		  cout << "LOWER_BOUND is now " << LOWER_BOUND << endl;
                }
		if(arg == "sinkson") {
		  cline >> arg;
                  param->sinkson = stoi(arg);
		  init_with_params(param);
		  cout << "USE_SINKS is now " << (USE_SINKS==true ? "true" : "false") << endl;
                }
		if(arg == "blueblue") {
		  cline >> arg;
                  param->blueblue = stoi(arg);
		  init_with_params(param);
		  cout << "MERGE_BLUE_BLUE is now " << (MERGE_BLUE_BLUE==true ? "true" : "false") << endl;
                }
		if(arg == "shallowfirst") {
		  cline >> arg;
                  param->shallowfirst = stoi(arg);
		  init_with_params(param);
		  // TODO: NOT RE-IMPLEMENTED YET 
		  cout << "SHALLOW_FIRST is now " << (MERGE_BLUE_BLUE==true ? "true" : "false") << endl;
                }
		if(arg == "largestblue") {
		  cline >> arg;
                  param->largestblue = stoi(arg);
		  init_with_params(param);
		  cout << "MERGE_MOST_VISITED is now " << (MERGE_MOST_VISITED==true ? "true" : "false") << endl;
                }
	      } else if(arg == "force") {
                // implements are mandatory merge
                cout << "State two sequences in abg format, ending in the same state: " << endl;
                string seq1 = "";
                string seq2 = "";


	      } else if(arg == "help") {
		cout << "Available commands: set <param> value, undo, help; insert <sample> in abd format; <int> merges the <int>th merge from the proposed list" << endl;
                // next command?
	        
	      } else if(arg == "insert") {
 	        // TODO: error checking
		cline >> arg;
	        merger->advance_apta(arg); 
	      } else {
	 
	        try {
		  choice = stoi(arg);
	          execute = true;
		  break;
                } catch(std::invalid_argument e) {
                  cout << "Invalid command. Try \"help\" if you are lost" << endl;
		  execute = false;
                } 
	      }
 
	    } else {
              //pos = 1;
              execute = true;
	    } // refs->size() 
          } //execute

          // track number of ops on APTA
          step++; 

          // auto-execute/leap steps
          if(countdown == 1) {
            manual = false;
            step = 1;
          }
          if(countdown > 0) {
            countdown--;
          }

	  // find chosen refinement and execute
          for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
            if(pos == choice) chosen_ref = *it;
              pos++;
          }
          pos = 1;     

            // execute choice 
            if(refs->empty()){
                cerr << "no more possible merges" << endl;
                break;
            }
            if(merger->red_states.size() > CLIQUE_BOUND){
               cerr << "too many red states" << endl;
               break;
            }
            // FIXME
            if(merger->get_final_apta_size() <= APTA_BOUND){
               cerr << "APTA too small" << endl;
               break;
            }

            /* if(GREEDY_METHOD == RANDOMG){
                merge_map randomized_merges;
                for(merge_map::reverse_iterator it = possible_merges->rbegin(); it != possible_merges->rend(); it++){
                    //if((*it).first < LOWER_BOUND) break;
                    randomized_merges.insert(pair<int, merge_pair>((*it).first * (rand() / (double)RAND_MAX), (*it).second));
                }
                top_score = (*randomized_merges.rbegin()).first;
                top_pair = (*randomized_merges.rbegin()).second;
            }*/

            // chosen ref instead of best ref
            chosen_ref->print_short();
            cerr << " ";

            chosen_ref->doref(merger);
            all_refs->push_front(chosen_ref);
            
            for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
                if(*it != chosen_ref) delete *it;
            }
            delete refs;
            num = num + 1;

	    execute = false;
        }
        cout << endl;

        int size =  merger->get_final_apta_size();
        int red_size = merger->red_states.size();
        cout << endl << "Found heuristic solution tion with " << size << " states, of which " << red_size << " are red states." << endl;
        return all_refs;
    }
    return all_refs;
};


