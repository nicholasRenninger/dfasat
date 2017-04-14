//#include <malloc.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "random_greedy.h"
#include "parameters.h"

merge_list random_greedy_bounded_run(state_merger* merger){
    cerr << "starting greedy merging" << endl;
    int num = 1;
    merge_list all_merges;
    while( true ){
        merger->reset();
        while( true ){
            cout << " ";
            // if(EXTEND_ANY_RED) while(merger->extend_red() != 0) cerr << "+ ";
            // leak here, too
            //merge_map* possible_merges = merger->get_possible_merges();

            refinement_set* refs = merger->get_possible_refinements();

            if(refs->empty()){
                cout << "no more possible merges" << endl;
                break;
            }
            if(merger->red_states.size() > CLIQUE_BOUND){
               cout << "too many red states" << endl;
               break;
            }
            // FIXME
            if(merger->get_final_apta_size() <= APTA_BOUND){
               cout << "APTA too small" << endl;
               break;
            }

            refinement best_ref = *refs->rbegin();
            /* if(GREEDY_METHOD == RANDOMG){
                merge_map randomized_merges;
                for(merge_map::reverse_iterator it = possible_merges->rbegin(); it != possible_merges->rend(); it++){
                    //if((*it).first < LOWER_BOUND) break;
                    randomized_merges.insert(pair<int, merge_pair>((*it).first * (rand() / (double)RAND_MAX), (*it).second));
                }
                top_score = (*randomized_merges.rbegin()).first;
                top_pair = (*randomized_merges.rbegin()).second;
            }*/
            cout << best_ref.score;
            best_ref.do_ref();
            all_merges.push_front(top_pair);
            
            delete possible_merges;
        }
        cout << endl;
        int size =  merger->get_final_apta_size();
        int red_size = merger->red_states.size();
        cout << endl << "found intermediate solution with " << size << " and " << red_size << " red states" << endl;
        return all_merges;
    }
    return all_merges;
};


