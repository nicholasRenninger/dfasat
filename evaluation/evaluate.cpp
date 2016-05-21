#include "state_merger.h"
#include "evaluate.h"
#include "evaluation_factory.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>
#include <string>
#include <sstream>

#include "parameters.h"

evaluation_data::evaluation_data(){
    node_type = -1;
    undo_pointer = 0;
};

void evaluation_data::read_from(int type, int index, int length, int symbol, string data){
    if(length == index){
        node_type = type;
    }
};

void evaluation_data::read_to(int type, int index, int length, int symbol, string data){
    if(length == index){
        node_type = type;
    }
};

void evaluation_data::update(evaluation_data* right){
    cerr << "read " << endl;
    if(node_type == -1){
        node_type = right->node_type;
        undo_pointer = right;
    }
};

void evaluation_data::undo(evaluation_data* right){
    if(right == undo_pointer){
        node_type = -1;
        undo_pointer = 0;
    }
};

/* default evaluation, count number of performed merges */
bool evaluation_function::consistent(state_merger *merger, apta_node* left, apta_node* right){
  if(inconsistency_found) return false;
  
  if(left->data->node_type != -1 && right->data->node_type != -1 && left->data->node_type != right->data->node_type){ inconsistency_found = true; return false; }
    
  return true;
};

void evaluation_function::update_score(state_merger *merger, apta_node* left, apta_node* right){
  num_merges += 1;
};

void evaluation_function::undo_update(state_merger *merger, apta_node* left, apta_node* right){
};

bool evaluation_function::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  return inconsistency_found == false;
};

int evaluation_function::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  return num_merges;
};

void evaluation_function::reset(state_merger *merger){
  inconsistency_found = false;
  num_merges = 0;
};

void evaluation_function::update(state_merger *merger){
};

void evaluation_function::initialize(state_merger *merger){
};

/* When is an APTA node a sink state?
 * sink states are not considered merge candidates
 *
 * accepting sink = only accept, accept now, accept afterwards
 * rejecting sink = only reject, reject now, reject afterwards 
 * low count sink = frequency smaller than STATE_COUNT */
bool is_low_count_sink(apta_node* node){
    node = node->find();
    return node->size < STATE_COUNT;
}

int evaluation_function::sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    if (is_low_count_sink(node)) return 0;
    //if (is_accepting_sink(node)) return 1;
    //if (is_rejecting_sink(node)) return 2;
    return -1;
};

bool evaluation_function::sink_consistent(apta_node* node, int type){
    if(!USE_SINKS) return false;
    
    if(type == 0) return is_low_count_sink(node);
    //if(type == 1) return is_accepting_sink(node);
    //if(type == 2) return is_rejecting_sink(node);
    
    return true;
};

int evaluation_function::num_sink_types(){
    if(!USE_SINKS) return 0;
    
    // accepting, rejecting, and low count
    return 1;
};

void evaluation_function::read_file(ifstream &input_stream, state_merger* merger){
    apta* aut = merger->aut;
    
    int num_words;
    int num_alph = 0;
    map<string, int> seen;
    int node_number = 1;
    input_stream >> num_words >> alphabet_size;
    
    for(int line = 0; line < num_words; line++){
        int type;
        int length;
        apta_node* node = aut->root;
        aut->root->depth = 0;
        input_stream >> type >> length;
        
        int depth = 0;
        for(int index = 0; index < length; index++){
            depth++;
            string tuple;
            input_stream >> tuple;
            
            std::stringstream lineStream;
            lineStream.str(tuple);
            
            string symbol;
            std::getline(lineStream,symbol,'/');
            string data;
            std::getline(lineStream,data);
            
            if(seen.find(symbol) == seen.end()){
                aut->alphabet[num_alph] = symbol;
                seen[symbol] = num_alph;
                num_alph++;
            }
            int c = seen[symbol];
            if(node->child(c) == 0){
                apta_node* next_node = new apta_node();
                node->children[c] = next_node;
                next_node->source = node;
                next_node->label  = c;
                next_node->number = node_number++;
                next_node->depth = depth;
            }
            node->size = node->size + 1;
            node->data->read_from(type, index, length, c, data);
            node = node->child(c);
            node->data->read_to(type, index, length, c, data);
        }
        if(depth > aut->max_depth) aut->max_depth = depth;
        node->type = type;
    }
};

void evaluation_function::print_dot(FILE* output, state_merger* merger){
    apta* aut = merger->aut;
    state_set candidates  = merger->get_candidate_states();
    
    fprintf(output,"digraph DFA {\n");
    fprintf(output,"\t%i [label=\"root\" shape=box];\n", aut->root->find()->number);
    fprintf(output,"\t\tI -> %i;\n", aut->root->find()->number);
    for(state_set::iterator it = merger->red_states.begin(); it != merger->red_states.end(); ++it){
        apta_node* n = *it;
        fprintf(output,"\t%i [shape=circle label=\"[%i]\"];\n", n->number, n->size);
        state_set childnodes;
        set<int> sinks;
        for(int i = 0; i < alphabet_size; ++i){
            apta_node* child = n->get_child(i);
            if(child == 0){
                // no output
            } else {
                 if(merger->sink_type(child) != -1){
                     sinks.insert(sink_type(child));
                 } else {
                     childnodes.insert(child);
                 }
            }
        }
        for(set<int>::iterator it2 = sinks.begin(); it2 != sinks.end(); ++it2){
            int stype = *it2;
            fprintf(output,"\tS%it%i [label=\"sink %i\" shape=box];\n", n->number, stype, stype);
            fprintf(output, "\t\t%i -> S%it%i [label=\"" ,n->number, n->number, stype);
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    fprintf(output, " %s [%i]", aut->alph_str(i).c_str(), n->size);
                }
            }
            fprintf(output, "\"];\n");
        }
        for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            fprintf(output, "\t\t%i -> %i [label=\"" ,n->number, child->number);
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    fprintf(output, " %s", aut->alph_str(i).c_str());
                }
            }
            fprintf(output, "\"];\n");
        }
    }
    for(state_set::iterator it = candidates.begin(); it != candidates.end(); ++it){
        apta_node* n = *it;
        if(sink_type(n) != -1);
            //fprintf(output,"\t%i [shape=box style=dotted label=\"[%i:%i]\"];\n", n->number, n->num_accepting, n->num_rejecting);
        else
            fprintf(output,"\t%i [shape=circle style=dotted label=\"[%i]\"];\n", n->number, n->size);
        for(int i = 0; i < alphabet_size; ++i){
            apta_node* child = n->get_child(i);
            if(child == 0){
                // no output
            } else if(sink_type(child) != -1) {
                /*int stype = sink_type(child);
                fprintf(output,"\tS%it%i [label=\"sink %i\" shape=box style=dotted];\n", n->number, stype, stype);
                fprintf(output, "\t\t%i -> S%it%i [label=\"%i [%i:%i]\" style=dotted];\n" ,n->number, n->number, stype, i, n->num_pos[i], n->num_neg[i]);*/
            } else {
                fprintf(output, "\t\t%i -> %i [label=\"%s [%i]\" style=dotted];\n" ,n->number, n->get_child(i)->number, aut->alph_str(i).c_str(), n->size);
            }
        }
    }
    /*for(state_set::iterator it = sinks.begin(); it != sinks.end(); ++it){
        apta_node* n = *it;
        if(sink_type(n) != -1)
            fprintf(output,"\t%i [shape=box style=dotted label=\"[%i:%i]\"];\n", n->number, n->num_accepting, n->num_rejecting);
        else if(n->num_accepting != 0)
            fprintf(output,"\t%i [shape=doublecircle style=dotted label=\"[%i:%i]\"];\n", n->number, n->num_accepting, n->num_rejecting);
        else if(n->num_rejecting != 0)
            fprintf(output,"\t%i [shape=Mcircle style=dotted label=\"[%i:%i]\"];\n", n->number, n->num_accepting, n->num_rejecting);
        else
            fprintf(output,"\t%i [shape=circle style=dotted label=\"[%i:%i]\"];\n", n->number, n->num_accepting, n->num_rejecting);
        for(int i = 0; i < alphabet_size; ++i){
            apta_node* child = n->get_child(i);
            if(child == 0){
                // no output
            } else {
                fprintf(output, "\t\t%i -> %i [label=\"%i [%i:%i]\" style=dotted];\n" ,n->number, n->get_child(i)->number, i, n->num_pos[i], n->num_neg[i]);
            }
        }
    }*/
    fprintf(output,"}\n");
};


