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
  merged_left_states.insert(left);
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
  merged_left_states.clear();
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
    if(!USE_SINKS) return true;
    
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

/*  read functions*/ 

void evaluation_function::init(string data, state_merger* merger) {

   // alphabet size is global but no longer used
   // once iterators are fully miplemented it's not needed
   // anymore. 
   std::stringstream lineStream;
   lineStream.str(data);

   int samples;
   lineStream >> samples >> alphabet_size;
 
   apta* aut = merger->aut;
   merger->node_number = 1;  
   aut->root->depth = 0;
 
}

void evaluation_function::add_sample(string data, state_merger* merger) { 


    // set up segmentation of sample line
    std::stringstream lineStream;
    lineStream.str(data);
           
    // header of line
    int label;
    int length;

    lineStream >> label >> length;

    apta* aut = merger->aut;

    apta_node* node = aut->root;
 

    // run over symbol/data of sample
    int depth=0;
    int num_alph=merger->seen.size();

    // leading BLANK between string and label length 
    string adv;
    std::getline(lineStream, adv, ' '); 

    // init with current length of seen
    for (int index=0; index < length; index++) {
        depth++;
        string symbol;
        string tuple;
        std::getline(lineStream,tuple,' ');
        string dat;

        std::stringstream elements;
        elements.str(tuple);

        std::getline(elements,symbol,'/');
        std::getline(elements,dat);

        //cout << "length: " << length << ". label: " << label << ". index" << index << ". symbol: " << symbol << ". data: " << dat << "." << endl;

         if(merger->seen.find(symbol) == merger->seen.end()){
             aut->alphabet[num_alph] = symbol;
             merger->seen[symbol] = num_alph;
             num_alph++;
         }
         int c = merger->seen[symbol];
         
         if(node->child(c) == 0){
             apta_node* next_node = new apta_node();
             node->children[c] = next_node;
             next_node->source = node;
             next_node->label  = c;
             next_node->number = merger->node_number++;
             next_node->depth = depth;
         }
         node->size = node->size + 1;
         node->data->read_from(label, index, length, c, dat);
         node = node->child(c);
         node->data->read_to(label, index, length, c, dat);
 
    }

    if(depth > aut->max_depth)  aut->max_depth = depth;

    node->type = label;

};

/* for batch mode */
// i want tis to map back to the sample by sample read functions from streaming
void evaluation_function::read_file(istream &input_stream, state_merger* merger){
    apta* aut = merger->aut;
    
    int num_words;
    int num_alph = 0;
    int node_number = 1;
    input_stream >> num_words >> alphabet_size;
    
    cerr << num_words << " " << alphabet_size << endl;
    
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
            
            if(merger->seen.find(symbol) == merger->seen.end()){
                aut->alphabet[num_alph] = symbol;
                merger->seen[symbol] = num_alph;
                num_alph++;
            }
            int c = merger->seen[symbol];
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
        if(depth > aut->max_depth)  aut->max_depth = depth;
     
        node->type = type;
  
    }
};

void evaluation_function::print_dot(iostream& output, state_merger* merger){
    apta* aut = merger->aut;
    state_set candidates  = aut->get_states();
    
    output << "digraph DFA {\n";
    output << "\t" << aut->root->find()->number << " [label=\"root\" shape=box];\n";
    output << "\t\tI -> " << aut->root->find()->number << ";\n";
    for(state_set::iterator it = merger->red_states.begin(); it != merger->red_states.end(); ++it){
        apta_node* n = *it;
        output << "\t" << n->number << " [shape=circle label=\"[" << n->size << "]\"];\n";
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
            output << "\tS" << n->number << "t" << stype << " [label=\"sink " << stype << "\" shape=box];\n";
            output << "\t\t" << n->number << " -> S" << n->number << "t" << stype << " [label=\"";
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    output << " " << aut->alph_str(i) << " [" << n->size << "]";
                }
            }
            output << "\"];\n";
        }
        for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            output << "\t\t" << n->number << " -> " << child->number << " [label=\""; 
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    output << " " << aut->alph_str(i);
                }
            }
            output << "\"];\n";
        }
    }
    for(state_set::iterator it = candidates.begin(); it != candidates.end(); ++it){
        apta_node* n = *it;
        if(sink_type(n) != -1);
            //fprintf(output,"\t%i [shape=box style=dotted label=\"[%i:%i]\"];\n", n->number, n->num_accepting, n->num_rejecting);
        else
            output << "\t" << n->number << " [shape=circle style=dotted label=\"[" << n->size << "]\"];\n";
        for(int i = 0; i < alphabet_size; ++i){
            apta_node* child = n->get_child(i);
            if(child == 0){
                // no output
            } else if(sink_type(child) != -1) {
                /*int stype = sink_type(child);
                fprintf(output,"\tS%it%i [label=\"sink %i\" shape=box style=dotted];\n", n->number, stype, stype);
                fprintf(output, "\t\t%i -> S%it%i [label=\"%i [%i:%i]\" style=dotted];\n" ,n->number, n->number, stype, i, n->num_pos[i], n->num_neg[i]);*/
            } else {
                output << "\t\t" << n->number << " -> " << n->get_child(i)->number << " [label=\"" << aut->alph_str(i) << " [" << n->size << "]\" style=dotted];\n";
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
    output << "}\n";
};

