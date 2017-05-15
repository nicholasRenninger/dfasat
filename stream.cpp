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

int stream_mode(state_merger* merger, parameters* param, ifstream& input_stream) {
       // first line has alphabet size and
       std::string line;
       std::getline(input_stream, line);
       merger->init_apta(line);

       // line by line processing
       // add items
       int i = 0;
       int solution = 0;

       merger->reset();

       std::getline(input_stream, line);
       merger->advance_apta(line);

       int samplecount = 1;

       // hoeffding countfor sufficient stats
       int hoeffding_count = (double)(RANGE*RANGE*log2(1/param->delta)) / (double)(2*param->epsilon*param->epsilon);
       cout << (RANGE*RANGE*log2(1/param->delta)) << " ";
       cout <<  (2*param->epsilon*param->epsilon);
       cout << " therefore, relevant Hoeffding count for " << (double) param->delta << " and " << (float)  param->epsilon << " delta/epsilon is " << hoeffding_count << endl;

       refinement_list* all_refs = new refinement_list();
       merger->eval->initialize(merger);

       while (std::getline(input_stream, line)) {
        merger->advance_apta(line);
        samplecount++;
        //merger.update_red_blue();

        if(samplecount % param->batchsize*hoeffding_count == 0) {
          while( true ) {
            cout << " ";

            refinement_set* refs = merger->get_possible_refinements();

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

            refinement* best_ref = *refs->begin();

            best_ref->print_short();
            cerr << " ";
            best_ref->doref(merger);
            all_refs->push_front(best_ref);

            for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
                if(*it != best_ref) delete *it;
            }
            delete refs;

          }
        } // if batchsize
      } // while input
}
