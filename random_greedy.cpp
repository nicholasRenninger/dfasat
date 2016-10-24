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
            cerr << " ";
            if(EXTEND_ANY_RED) while(merger->extend_red() == true) cout << "+ ";
	    // leak here, too
            merge_map *m = &merger->get_possible_merges();
            merge_map possible_merges = *m;//merger->get_possible_merges();
            delete m;
            if(!EXTEND_ANY_RED && possible_merges.empty()){
                if(merger->extend_red() == true) { cout << "+"; continue; }
                cout << "no more possible merges" << endl;
                break;
            }
            if(possible_merges.empty()){
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
            /*if((*possible_merges.rbegin()).first < LOWER_BOUND){
                cerr << "merge score below lower bound" << endl;
                break;
            }*/

            /*cerr << "possible merges: ";
            for(merge_map::reverse_iterator it = possible_merges.rbegin(); it != possible_merges.rend(); it++){
                cerr << (*it).first << " ";
            }
            cerr << endl;*/

            merge_pair top_pair = (*possible_merges.rbegin()).second;
            float top_score = (*possible_merges.rbegin()).first;
            if(GREEDY_METHOD == RANDOMG){
                merge_map randomized_merges;
                for(merge_map::reverse_iterator it = possible_merges.rbegin(); it != possible_merges.rend(); it++){
                    //if((*it).first < LOWER_BOUND) break;
                    randomized_merges.insert(pair<int, merge_pair>((*it).first * (rand() / (double)RAND_MAX), (*it).second));
                }
                top_score = (*randomized_merges.rbegin()).first;
                top_pair = (*randomized_merges.rbegin()).second;
            }
            cout << top_score;
            merger->perform_merge(top_pair.first, top_pair.second);
            all_merges.push_front(top_pair);
        }
        cout << endl;
        int size = merger->get_final_apta_size();
        int red_size = merger->red_states.size();
        cout << "\nfound intermediate solution with size " << size << " and " << red_size << " red states" << endl;
        return all_merges;
    }
    return all_merges;
};


