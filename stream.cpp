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

int STREAM_COUNT = 0;

int stream_mode(state_merger* merger, parameters* param, ifstream& input_stream) {
       // first line has alphabet size and
       std::string line;
       //std::getline(input_stream, line);
       merger->init_apta(string("10 1000"));

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

       STREAM_COUNT = hoeffding_count;
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

            // output each micro-batch, keeping at host 10 files
            merger->todot();
            std::ostringstream oss2;
            oss2 << param->dot_file <<"stream_pre_" << samplecount++ % 10 << ".dot";
            ofstream output(oss2.str().c_str());
            //if(output == NULL) cerr << "Cannot open " << oss2.str().c_str() << " to write intermediate dot output file" << endl;
            output << merger->dot_output;
            output.close();


            refinement_set* refs = merger->get_possible_refinements();

            if(refs->empty()){
                // we just need more data
                // cerr << "no more possible merges" << endl;
                break;
            }
            // the following flags are mostly important for the sat solver
            if(merger->red_states.size() > CLIQUE_BOUND){
               cerr << "too many red states" << endl;
               break;
            }
            // FIXME
            if(merger->get_final_apta_size() <= APTA_BOUND){
               cerr << "APTA too small" << endl;
               break;
            }

            // top refinements to compare
            refinement* best_ref;
            bool found = true;

            if(refs->size() > 1) {

              std::set<refinement*, score_compare>::iterator top_ref_it = refs->begin();
              std::set<refinement*, score_compare>::iterator runner_up_ref_it = refs->begin();

              refinement* top_ref = *top_ref_it;
              refinement* runner_up_ref = *( ++(runner_up_ref_it));

              // enough evidence take best score and compare
              while( ! (top_ref_it == refs->end()) ) {
                // where's the match;
                for(runner_up_ref_it = std::next(top_ref_it); runner_up_ref_it != refs->end() && top_ref->score - runner_up_ref->score <  param->epsilon; ++runner_up_ref_it) {

                  if(runner_up_ref->right != top_ref->right) {
                    continue;
                  } else {
                    if(top_ref->score - runner_up_ref->score > param->epsilon) {
                      found = true;
                      break;
                    } else {
                        cout << " [" << top_ref->score << ":" << runner_up_ref->score << "] ";
                        // we are not cnofident in top merge
                        found = false;
                    }
                  }
                  runner_up_ref = *runner_up_ref_it;
                }

                // current top score is hoeffding match for this blue state
                best_ref = top_ref;
                if(found) break;

                // check if next best score is hoeffding match
                top_ref = *(++top_ref_it);
              } // end find matching

            } else {
              // execute if enough evidence?
              best_ref = *(refs->begin());
            } // end only one ref

            // what if we have no confident in any proposed refinement?
            if(found==false) {
              best_ref = *(refs->begin());
              //cerr << "no option" << endl;
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

      cout << endl;
      int size =  merger->get_final_apta_size();
      int red_size = merger->red_states.size();
      cout << endl << "found intermediate solution with " << size << " and " << red_size << " red states" << endl;

      return 0;
}
