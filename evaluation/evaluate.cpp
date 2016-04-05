#include "state_merger.h"
#include "evaluate.h"
#include "evaluation_factory.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

evaluation_data::evaluation_data(){
    node_type = -1;
    undo_pointer = 0;
};

void evaluation_data::read(int type, int index, int length, int symbol, string data){
    if(length == index){
        node_type = type;
    }
};

void evaluation_data::update(evaluation_data* right){
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
  
  if(left->data->node_type != right->data->node_type){ inconsistency_found = true; return false; }
    
  return true;
};

void evaluation_function::update_score(state_merger *merger, apta_node* left, apta_node* right){
  num_merges += 1;
};

bool evaluation_function::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  return inconsistency_found == false && compute_score(merger, left, right) > LOWER_BOUND;
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
    return node->accepting_paths + node->num_accepting < STATE_COUNT;
}

bool is_accepting_sink(apta_node* node){
    node = node->find();
    return return node->rejecting_paths == 0 && node->num_rejecting == 0;
}

bool is_rejecting_sink(apta_node* node){
    node = node->find();
    return node->accepting_paths == 0 && node->num_accepting == 0;
}

int evaluation_function::sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    if (is_low_count_sink(node)) return 0;
    if (is_accepting_sink(node)) return 1;
    if (is_rejecting_sink(node)) return 2;
    return -1;
};

bool evaluation_function::sink_consistent(apta_node* node, int type){
    if(!USE_SINKS) return false;
    
    if(type == 0) return is_low_count_sink(node);
    if(type == 1) return is_accepting_sink(node);
    if(type == 2) return is_rejecting_sink(node);
    
    return true;
};

int evaluation_function::num_sink_types(){
    if(!USE_SINKS) return 0;
    
    // accepting, rejecting, and low count
    return 3;
};

void evaluation_function::read_file(FILE* input, state_merger* merger){
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

            node->data->read(type, index, length, c, data);

            if(node->child(c) == 0){
                apta_node* next_node = new apta_node();
                node->children[c] = next_node;
                next_node->source = node;
                next_node->label  = c;
                next_node->number = node_number++;
                next_node->depth = depth;
            }
            node = node->child(c);
        }
        if(depth > max_depth) max_depth = depth;
        node->type = type;
    }
};

void evaluation_function::print_dot(FILE* output, state_merger* merger){
    apta* aut = merger->aut;
    state_set candidates  = merger->get_candidate_states();
    
    //state_set sinks = get_sink_states();

    fprintf(output,"digraph DFA {\n");
    fprintf(output,"\t%i [label=\"root\" shape=box];\n", aut->root->find()->number);
    fprintf(output,"\t\tI -> %i;\n", aut->root->find()->number);
    for(state_set::iterator it = merger->red_states.begin(); it != merger->red_states.end(); ++it){
        apta_node* n = *it;

        double error = 0.0;
        double mean = 0.0;

        for(double_list::iterator it = n->occs.begin(); it != n->occs.end(); ++it){
             mean = mean + (double)*it;
        }
        mean = mean / (double)n->occs.size();

        for(double_list::iterator it = n->occs.begin(); it != n->occs.end(); ++it){
            error = error + ((mean - (double)*it)*(mean - (double)*it));
        }
        error = error / (double)n->occs.size();
        
        int node_size = (float)(n->accepting_paths + n->rejecting_paths)/2.0;

        if(n->num_accepting != 0)
            fprintf(output,"\t%i [shape=ellipse style=\"filled\" color=\"green\" label=\"%i\"];\n", n->number, n->num_accepting);
        else if(n->num_rejecting != 0)
            fprintf(output,"\t%i [shape=ellipse style=\"filled\" color=\"red\" label=\"%i\"];\n", n->number, n->num_rejecting);
        else {
            if(mean < 4)
                fprintf(output,"\t%i [shape=ellipse penwidth=%i style=\"filled\" color=\"red\" label=\"\n%.3f\n%i\"];\n", n->number, node_size, mean, (int)n->occs.size());
            else if(mean < 6)
                fprintf(output,"\t%i [shape=ellipse penwidth=%i style=\"filled\" color=\"purple\" label=\"\n%.3f\n%i\"];\n", n->number, node_size, mean, (int)n->occs.size());
            else if(mean < 7.5)
                fprintf(output,"\t%i [shape=ellipse penwidth=%i style=\"filled\" color=\"yellow\" label=\"\n%.3f\n%i\"];\n", n->number, node_size, mean, (int)n->occs.size());
            else if(mean < 10.0)
                fprintf(output,"\t%i [shape=ellipse penwidth=%i style=\"filled\" color=\"green\" label=\"\n%.3f\n%i\"];\n", n->number, node_size, mean, (int)n->occs.size());
            else
                fprintf(output,"\t%i [shape=ellipse penwidth=%i penlabel=\"\"];\n", n->number, node_size);
        }
        state_set childnodes;
        set<int> sinks;
        for(int i = 0; i < alphabet_size; ++i){
            apta_node* child = n->get_child(i);
            if(child == 0){
                // no output
            } else {
                 if(sink_type(child) != -1){
                     sinks.insert(sink_type(child));
                 } else {
            int node_size = (float)(n->pos(i) + n->neg(i)) + 1;
            if (aut->alph_str(i) == "<0>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"black\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<1>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"black\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<2>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<3>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"green\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<4>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<5>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<6>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<7>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"green\"];\n" ,n->number, child->number, node_size);
            if (aut->alph_str(i) == "<8>")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"green\"];\n" ,n->number, child->number, node_size);

                     //childnodes.insert(child);
                 }
            }
        }
        for(set<int>::iterator it2 = sinks.begin(); it2 != sinks.end(); ++it2){
            int stype = *it2;
            fprintf(output,"\tS%it%i [label=\"sink %i\" shape=box];\n", n->number, stype, stype);
            fprintf(output, "\t\t%i -> S%it%i [label=\"" ,n->number, n->number, stype);
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    fprintf(output, " %s [%i:%i]", aut->alph_str(i).c_str(), n->num_pos[i], n->num_neg[i]);
                }
            }
            fprintf(output, "\"];\n");
        }
        /*for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            fprintf(output, "\t\t%i -> %i [label=\"" ,n->number, child->number);
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    fprintf(output, " %s [%i:%i]", aut->alph_str(i).c_str(), n->num_pos[i], n->num_neg[i]);
                }
            }
            fprintf(output, "\"];\n");
        }*/
    }
    for(state_set::iterator it = candidates.begin(); it != candidates.end(); ++it){
        apta_node* n = *it;
        if(sink_type(n) != -1);
            //fprintf(output,"\t%i [shape=box style=dotted label=\"[%i:%i]\"];\n", n->number, n->num_accepting, n->num_rejecting);
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
            } else if(sink_type(child) != -1) {
                /*int stype = sink_type(child);
                fprintf(output,"\tS%it%i [label=\"sink %i\" shape=box style=dotted];\n", n->number, stype, stype);
                fprintf(output, "\t\t%i -> S%it%i [label=\"%i [%i:%i]\" style=dotted];\n" ,n->number, n->number, stype, i, n->num_pos[i], n->num_neg[i]);*/
            } else {
                fprintf(output, "\t\t%i -> %i [label=\"%s [%i:%i]\" style=dotted];\n" ,n->number, n->get_child(i)->number, aut->alph_str(i).c_str(), n->num_pos[i], n->num_neg[i]);
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


