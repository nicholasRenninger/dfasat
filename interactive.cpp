//#include <malloc.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "refinement.h"
#include "random_greedy.h"
#include "parameters.h"

refinement_list* interactive(state_merger* merger, parameters* param){
    cerr << "starting greedy merging" << endl;
    int num = 1;
    refinement_list* all_refs = new refinement_list();
    merger->eval->initialize(merger);

    int choice = 1;
    int pos = 0;
    // if there is only one choice, do we auto-execute
    // or read for confirmation & chance for parameter changes?
    bool manual = false;

    while( true ){
        merger->reset();
        while( true ){
            cout << " ";
            
            merger->todot();
            std::ostringstream oss2;
            oss2 << param->dot_file <<"pre_" << num % 2 << ".dot";
            ofstream output(oss2.str().c_str());
            output << merger->dot_output;
            output.close();
           
            refinement_set* refs = merger->get_possible_refinements();
            refinement* chosen_ref = *refs->begin();

            cerr << endl;

            // if there is more than one option
	    if(refs->size() > 1 && !manual) {

              cout << "Possible refinements: " << endl;
              for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
                  (*it)->print_short();
                  cout << " , ";
              }

              cout << endl <<"Your choice : " << endl;
              cin >> choice;
 
              for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
                  if(pos == choice) chosen_ref = *it;
                  pos++;
              }
              pos = 1;
            }

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
        }
        cout << endl;

        int size =  merger->get_final_apta_size();
        int red_size = merger->red_states.size();
        cout << endl << "found intermediate solution with " << size << " and " << red_size << " red states" << endl;
        return all_refs;
    }
    return all_refs;
};


