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

void evaluation_data::print_state_label(iostream& output){

};

void evaluation_data::print_state_style(iostream& output){

};

void evaluation_data::print_transition_label(iostream& output, apta_node* child){

};

void evaluation_data::print_transition_style(iostream& output, apta_node* child){

};

void evaluation_data::print_sink_label(iostream& output, int type){

};

void evaluation_data::print_sink_style(iostream& output, int type){

};

void evaluation_data::print_sink_transition_label(iostream& output, int type){

};

void evaluation_data::print_sink_transition_style(iostream& output, int type){

};

int evaluation_data::sink_type(){
    return -1;
};

bool evaluation_data::sink_consistent(int type){   
    return true;
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
         
 
         // if root is red, mark child blue
         if(node->child(c) == 0){
             apta_node* next_node = new apta_node(aut);
             node->children[c] = next_node;
             next_node->source = node;
             next_node->label  = c;
             next_node->number = merger->node_number++;
             next_node->depth = depth;
  
             // move this down
             if(merger->red_states.find(node) != merger->red_states.end()) { merger->blue_states.insert(node->child(c)); cout << "FOUND: add " << next_node << endl; }
         }
         node->size = node->size + 1;
         node->data->read_from(label, index, length, c, dat);
         node = node->child(c);
         node->data->read_to(label, index, length, c, dat);

         // update union-find structure
         // next_node->find() 
    }

    if(depth > aut->max_depth)  aut->max_depth = depth;

    node->type = label;

};


