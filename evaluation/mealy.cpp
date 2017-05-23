#include "state_merger.h"
#include "evaluate.h"
#include "evaluation_factory.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "parameters.h"
#include "mealy.h"

REGISTER_DEF_TYPE(mealy);
REGISTER_DEF_DATATYPE(mealy_data);

int mealy_data::num_outputs;
si_map mealy_data::output_int;
is_map mealy_data::int_output;

void mealy_data::read_from(int type, int index, int length, int symbol, string data){
    //cerr << "read: " << symbol << "/" << data << endl;
    evaluation_data::read_from(type, index, length, symbol, data);
    if(output_int.find(data) == output_int.end()){
       output_int[data] = num_outputs;
       int_output[num_outputs] = data;
       num_outputs++;
    }
    outputs[symbol] = output_int[data];
};

void mealy_data::update(evaluation_data* right){
    mealy_data* other = reinterpret_cast<mealy_data*>(right);
    
    for(output_map::iterator it = other->outputs.begin(); it != other->outputs.end(); ++it){
        int input  = (*it).first;
        int output = (*it).second;
        
        if(outputs.find(input) == outputs.end()){
            outputs[input] = output;
            undo_info[input] = other;
        }
    }
};

void mealy_data::undo(evaluation_data* right){
    mealy_data* other = reinterpret_cast<mealy_data*>(right);

    for(output_map::iterator it = other->outputs.begin(); it != other->outputs.end(); ++it){
        int input  = (*it).first;
        int output = (*it).second;
        
        undo_map::iterator it2 = undo_info.find(input);
        
        if(it2 != undo_info.end() && (*it2).second == other){
            outputs.erase(input);
            undo_info.erase(input);
        }
    }
};

/* default evaluation, count number of performed merges */
bool mealy::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(inconsistency_found) return false;
  
    mealy_data* l = reinterpret_cast<mealy_data*>(left->data);
    mealy_data* r = reinterpret_cast<mealy_data*>(right->data);
    
    int matched = 0;
    
    for(output_map::iterator it = r->outputs.begin(); it != r->outputs.end(); ++it){
        int input  = (*it).first;
        int output = (*it).second;

        if(l->outputs.find(input) != l->outputs.end()){
            if(l->outputs[input] != output){
                inconsistency_found = true;
                return false;
            }
            matched = matched + 1;
        }
    }
    
    num_unmatched = num_unmatched + (l->outputs.size() - matched);
    num_unmatched = num_unmatched + (r->outputs.size() - matched);
    num_matched   = num_matched   + matched;
    
    return true;
};

bool mealy::compute_consistency(state_merger* merger, apta_node* left, apta_node* right){
    if(evaluation_function::compute_consistency(merger, left, right) == false) return false;
    return true;
};

int mealy::compute_score(state_merger* merger, apta_node* left, apta_node* right){
    return num_matched;
};

void mealy::reset(state_merger* merger){
    evaluation_function::reset(merger);
    num_matched = 0;
    num_unmatched = 0;
};


void mealy_data::print_transition_label(iostream& output, int symbol, apta* aptacontext){

};


bool mealy_data::is_stream_sink(apta_node* node) {

   return node->size < STREAM_COUNT;
}

bool mealy_data::is_low_count_sink(apta_node* node) {

   return node->size < STATE_COUNT;
}



int mealy_data::sink_type(apta_node* node){

    if(is_stream_sink(node)) return 2;

    if(!USE_SINKS) return -1;

    if(is_low_count_sink(node)) return 0;

    return -1;
};

bool mealy_data::sink_consistent(int type){
    if(!USE_SINKS) return true;
    
    //if(type == 0) return is_low_count_sink();
    //if(type == 1) return this->is_stream_sink();
    return true;
};

int mealy::num_sink_types(){
    if(!USE_SINKS) return 0;
    return 2;
};



/*void mealy::print_dot(iostream& output, state_merger* merger){
    apta* aut = merger->aut;
    state_set s  = merger->red_states;
    
    cerr << "size: " << s.size() << endl;
    
    output << "digraph DFA {\n";
    output << "\t" << aut->root->find()->number << " [label=\"root\" shape=box];\n";
    output << "\t\tI -> " << aut->root->find()->number << ";\n";
    for(state_set::iterator it = s.begin(); it != s.end(); ++it){
        apta_node* n = *it;
        mealy_data* l = reinterpret_cast<mealy_data*>(n->data);
        output << "\t" << n->number << " [shape=circle label=\"" << n->size << "\"];\n";
        
        state_set childnodes;
        set<int> sinks;
        for(int i = 0; i < alphabet_size; ++i){
            apta_node* child = n->get_child(i);
            if(child == 0){
                // no output
            } else {
                 childnodes.insert(child);
            }
        }
        for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            output << "\t\t" << n->number << " -> " << child->number << " [label=\"";
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    output << " " << aut->alph_str(i) << "/" << mealy_data::int_output[l->outputs[i]];
                }
            }
            output << "\"];\n";
        }
    }

    s = merger->get_candidate_states();
    for(state_set::iterator it = s.begin(); it != s.end(); ++it){
        apta_node* n = *it;
        mealy_data* l = reinterpret_cast<mealy_data*>(n->data);
        output << "\t" << n->number << " [shape=circle label=\"" << n->size << "\"];\n";
        for(child_map::iterator it2 = n->children.begin(); it2 != n->children.end(); ++it2){
            apta_node* child = (*it2).second;
            int symbol = (*it2).first;
            output << "\t\t" << n->number << " -> " << child->number << " [style=dotted label=\"" << aut->alph_str(symbol) << "\\\\" << mealy_data::int_output[l->outputs[symbol]] << "\"];\n";
        }
    }
    output << "}\n";
};*/

