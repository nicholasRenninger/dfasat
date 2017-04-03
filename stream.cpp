#include <stdlib.h>
#include <popt.h>
#include "random_greedy.h"
#include "state_merger.h"
#include "evaluate.h"
#include "dfasat.h"
#include <sstream>
#include <iostream>
#include "evaluation_factory.h"
#include <string>
#include "searcher.h"
#include <vector>
#include <cmath>

#include "parameters.h"

int stream_mode(state_merger merger, parameters* param, ifstream& input_stream) {
       // first line has alphabet size and
       std::string line;
       std::getline(input_stream, line);
       merger.init_apta(line);

       // line by line processing
       // add items
       int i = 0;
       int solution = 0;
       merge_list all_merges;

       merger.reset();

       std::getline(input_stream, line);
       merger.advance_apta(line);

       int samplecount = 1;

       // hoeffding countfor sufficient stats
       int hoeffding_count = (RANGE*RANGE*log2(1/param->delta)) / (2*param->epsilon*param->epsilon);
       cout << (RANGE*RANGE*log2(1/param->delta)) << " ";
       cout <<  (2*param->epsilon*param->epsilon);
       cout << "Relevant Hoeffding count for " << (double) param->delta << " and " << (float)  param->epsilon << " delta/epsilon is " << hoeffding_count << endl;

       while (std::getline(input_stream, line)) {
        merger.advance_apta(line);
        samplecount++;
        //merger.update_red_blue();

        merge_list all_merges;

        if(samplecount % param->batchsize*hoeffding_count == 0) {
           while( true ) {
               cout << " ";
               if(EXTEND_ANY_RED) while(merger.extend_red() != 0) {
                   cerr << "+ ";
               }

               merge_map* possible_merges = merger.get_possible_merges(hoeffding_count);

               if(!EXTEND_ANY_RED && possible_merges->empty()){
                   if(merger.extend_red() != 0) { cerr << "+"; continue; }
                   cout << "~ ";//"no more possible merges with extend any red";
                   break;
               }
               if(possible_merges->empty()){
                   cout << "- "; // "no more possible merges " << merger.blue_states.size();
                   break;
               }
               if(merger.red_states.size() > CLIQUE_BOUND){
                   cout << "too many red states " << merger.red_states.size();
                  break;
               }

               merge_pair top_pair = (*possible_merges->rbegin()).second;
               float top_score = (*possible_merges->rbegin()).first;

               merge_pair runnerup_pair;
               float runnerup_score = -1;

               if(possible_merges->size() > 1) {
                   runnerup_score = (*(++(possible_merges->rbegin()))).first;
               } else {

               }

	   // random-greedy scales all scores by random number
            /*if(GREEDY_METHOD == RANDOMG){
                merge_map randomized_merges;
                for(merge_map::reverse_iterator it = possible_merges->rbegin(); it != possible_merges->rend(); it++){
                    //if((*it).first < LOWER_BOUND) break;
                    randomized_merges.insert(pair<int, merge_pair>((*it).first * (rand() / (double)RAND_MAX), (*it).second));
                }
                top_score = (*randomized_merges.rbegin()).first;
                top_pair = (*randomized_merges.rbegin()).second;
            }*/

	    // if heuristic requirement basedo n Hoeffding bound is true, i.e.
	    // if difference between top and second-to-top
               if(top_score - runnerup_score > param->epsilon) {
                 merger.perform_merge(top_pair.first, top_pair.second);
                 all_merges.push_front(top_pair);
               } else {
                 cout << "< "; // not large enough top score
                 break;
               }

	       cout << "("  << top_score << " " << runnerup_score << ") ";
               delete possible_merges;

           } // while true

        int size =  merger.get_final_apta_size();
        int red_size = merger.red_states.size();
        cout << " X " ;
        // cout << endl << "found intermediate solution with " << size << " and " << red_size << " red states" << endl;
      }

     } // if batchsize
} // function
