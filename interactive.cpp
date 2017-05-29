/// @file
/// \brief All the functions and definitions for interactive merge mode.

#include <stdio.h>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <sstream>

#include "refinement.h"
#include "random_greedy.h"
#include "parameters.h"

/*! \brief Main loop for interactive mode.
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
    int choice = 1;
    int pos = 0;

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

              cout << endl << "Your choice : " << endl;
              getline(std::cin, command);
	      stringstream cline(command);
              cline >> arg;

	      // parse (prefix-free) commands 
	      if(arg  == "undo") {
                (*all_refs->begin())->undo(merger);
                all_refs->pop_front();
                cout << "undid last merge" << endl;
	      } else if(arg == "set") {
	        // set PARAM <value>
	          // simple modify the param structure and call init_with_params, done

		cline >> arg;

		if(arg == "state_count") {
		  cline >> arg;
                  param->state_count = stoi(arg);
		  init_with_params(param);
		  cout << "STATE_COUNT is now " << STATE_COUNT << endl;
                }

	      } else if(arg == "next") {

	      } else if(arg == "help") {
		cout << "Available commands: set <param> value, undo, help; insert <sample> in abd format; <int> merges the <int>th merge from the proposed list" << endl;
                // next command?
	        
	      } else if(arg == "insert") {
 	        // TODO: error checking
		cline >> arg;
	        merger->advance_apta(arg); 
	      } else {
		execute = true;
                // TODO Error handling
		pos = stoi(arg);
                break;
	      }
 
	    } else {
              pos = 1;
              execute = true;
             
	    } // refs->size() 
          } //execute

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


