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

       int batch = 0;

       while (std::getline(input_stream, line)) {
        merger->advance_apta(line);


        samplecount++;
        batch++;
        //merger.update_red_blue();

        if(samplecount % param->batchsize == 0) {
          while( true ) {

            merger->todot();
            std::ostringstream oss2;
            oss2 << "stream_pre_" << samplecount++ << ".dot";
            ofstream output(oss2.str().c_str());
            output << merger->dot_output;
            output.close();


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

            // top refinements to compare?
            if(refs->size() > 1) {

              // go find nex t blue state or up to blue
              refinement* runner_up = *( ++(refs->begin()));

              // enough evidence?
              if( (best_ref->score - runner_up->score) > param->epsilon) {
                // implement merge
              } else {
                // we need more data
                cerr << best_ref->score << " " << runner_up->score << " ";
                //break;
              }

            } else {
              // only one ref
              //cerr << "ref size  " << refs->size() << " ";
            }

            if(batch > 1) {
              cerr << "b" << batch << " ";
            }
            best_ref->print_short();
            cerr << " ";
            best_ref->doref(merger);
            all_refs->push_front(best_ref);
            batch = 0;

            for(refinement_set::iterator it = refs->begin(); it != refs->end(); ++it){
                if(*it != best_ref) delete *it;
            }
            delete refs;

          }
        } // if batchsize
      } // while input

      // do one last step of batch?

      // remaining input not used
      cerr << "b" << batch << " ";
      return 0;
}
