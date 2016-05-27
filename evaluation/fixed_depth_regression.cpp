#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "fixed_depth_regression.h"

REGISTER_DEF_DATATYPE(fixed_depth_mse_data);
REGISTER_DEF_TYPE(fixed_depth_mse_error);

fixed_depth_mse_data::fixed_depth_mse_data(){
    num_accepting = 0;
    num_rejecting = 0;
    accepting_paths = 0;
    rejecting_paths = 0;
    num_pos = num_map();
    num_neg = num_map();
};

void fixed_depth_mse_data::read_from(int type, int index, int length, int symbol, string data){
    double occ = std::stod(data);
    if(occ > 0.0){
        mean = ((mean * ((double)occs.size())) + occ) / ((double)(occs.size() + 1));
        occs.push_front(occ);
    }
    if(type == 1){
        accepting_paths++;
    } else {
        rejecting_paths++;
    }
    if(type == 1){
        num_pos[symbol] = pos(symbol) + 1;
    } else {
        num_neg[symbol] = neg(symbol) + 1;
    }
};

void fixed_depth_mse_data::read_to(int type, int index, int length, int symbol, string data){
    mse_data::read_to(type, index, length, symbol, data);
    if(type == 1){
        if(length == index+1){
            num_accepting++;
        }
    } else {
        if(length == index+1){
            num_rejecting++;
        }
    }
};

void fixed_depth_mse_data::update(evaluation_data* right){
    mse_data::update(right);
    fixed_depth_mse_data* other = (fixed_depth_mse_data*)right;
    num_accepting += other->num_accepting;
    num_rejecting += other->num_rejecting;
    accepting_paths += other->accepting_paths;
    rejecting_paths += other->rejecting_paths;
    for(num_map::iterator it = other->num_pos.begin();it != other->num_pos.end(); ++it){
        num_pos[(*it).first] = pos((*it).first) + (*it).second;
    }
    for(num_map::iterator it = other->num_neg.begin();it != other->num_neg.end(); ++it){
        num_neg[(*it).first] = neg((*it).first) + (*it).second;
    }
};

void fixed_depth_mse_data::undo(evaluation_data* right){
    mse_data::undo(right);
    fixed_depth_mse_data* other = (fixed_depth_mse_data*)right;
    num_accepting -= other->num_accepting;
    num_rejecting -= other->num_rejecting;
    accepting_paths -= other->accepting_paths;
    rejecting_paths -= other->rejecting_paths;
    for(num_map::iterator it = other->num_pos.begin();it != other->num_pos.end(); ++it){
        num_pos[(*it).first] = pos((*it).first) - (*it).second;
    }
    for(num_map::iterator it = other->num_neg.begin();it != other->num_neg.end(); ++it){
        num_neg[(*it).first] = neg((*it).first) - (*it).second;
    }
};

bool fixed_depth_mse_error::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(left->depth != right->depth){ inconsistency_found = true; return false; }
    
    fixed_depth_mse_data* l = (fixed_depth_mse_data*)left;
    fixed_depth_mse_data* r = (fixed_depth_mse_data*)right;
    
    if(l->occs.size() == 0 && r->occs.size() != 0){ inconsistency_found = true; return false; }
    if(l->occs.size() != 0 && r->occs.size() == 0){ inconsistency_found = true; return false; }

    //if(l->num_accepting != 0 && r->num_rejecting != 0){ inconsistency_found = true; return false; }
    //if(l->num_rejecting != 0 && r->num_accepting != 0){ inconsistency_found = true; return false; }
    return mse_error::consistent(merger,left,right);
};

void fixed_depth_mse_error::print_dot(FILE* output, state_merger* merger){
    apta* aut = merger->aut;
    state_set candidates  = merger->get_candidate_states();
    
    //state_set sinks = get_sink_states();

    fprintf(output,"digraph DFA {\n");
    
    fprintf(output,"subgraph good {\n");
    state_set* state = &merger->aut->get_merged_states();
    state_set states = *state;
    for(state_set::iterator it = states.begin(); it != states.end(); ++it){
        apta_node* node = *it;
        fixed_depth_mse_data* l = (fixed_depth_mse_data*) node->data;
        if (l->accepting_paths > 2.0 * l->rejecting_paths){// || l->num_accepting != 0){
            fprintf(output,"%i ",node->number);
        }
    }
    delete state;
    fprintf(output,";\nlabel = \"good\";\n");
    fprintf(output,"}\n");
    fprintf(output,"subgraph bad {\n");
    state = &merger->aut->get_merged_states();
    states = *state;
    for(state_set::iterator it = states.begin(); it != states.end(); ++it){
        apta_node* node = *it;
        fixed_depth_mse_data* l = (fixed_depth_mse_data*) node->data;
        if (l->rejecting_paths > 2.0 * l->accepting_paths){// || l->num_rejecting != 0){
            fprintf(output,"%i ",node->number);
        }
    }
    delete state;
    fprintf(output,";\nlabel = \"bad\";\n");
    fprintf(output,"}\n");
    

    fprintf(output,"\t%i [label=\"root\" shape=box];\n", aut->root->find()->number);
    fprintf(output,"\t\tI -> %i;\n", aut->root->find()->number);
    for(state_set::iterator it = merger->red_states.begin(); it != merger->red_states.end(); ++it){
        apta_node* n = *it;
        fixed_depth_mse_data* l = reinterpret_cast<fixed_depth_mse_data*>(n->data);

        int node_size = 1 + (int)((float)(l->occs.size())/10.0);

        if(l->num_accepting != 0)
            fprintf(output,"\t%i [shape=box style=\"filled\" color=\"green\" label=\"%i\"];\n", n->number, l->num_accepting);
        else if(l->num_rejecting != 0)
            fprintf(output,"\t%i [shape=box style=\"filled\" color=\"red\" label=\"%i\"];\n", n->number, l->num_rejecting);
        else {
            if(l->occs.size() == 0)
                fprintf(output,"\t%i [shape=box width=%i style=\"filled\" color=\"cyan\" label=\"\n%.3f\n%i\n%i:%i\"];\n", n->number, node_size, l->mean, (int)l->occs.size(), l->accepting_paths, l->rejecting_paths);
            else if(l->mean < 4)
                fprintf(output,"\t%i [shape=box width=%i style=\"filled\" color=\"red\" label=\"\n%.3f\n%i\n%i:%i\"];\n", n->number, node_size, l->mean, (int)l->occs.size(), l->accepting_paths, l->rejecting_paths);
            else if(l->mean < 6)
                fprintf(output,"\t%i [shape=box width=%i style=\"filled\" color=\"purple\" label=\"\n%.3f\n%i\n%i:%i\"];\n", n->number, node_size, l->mean, (int)l->occs.size(), l->accepting_paths, l->rejecting_paths);
            else if(l->mean < 7.5)
                fprintf(output,"\t%i [shape=box width=%i style=\"filled\" color=\"yellow\" label=\"\n%.3f\n%i\n%i:%i\"];\n", n->number, node_size, l->mean, (int)l->occs.size(), l->accepting_paths, l->rejecting_paths);
            else if(l->mean < 10.0)
                fprintf(output,"\t%i [shape=box width=%i style=\"filled\" color=\"green\" label=\"\n%.3f\n%i\n%i:%i\"];\n", n->number, node_size, l->mean, (int)l->occs.size(),  l->accepting_paths, l->rejecting_paths);
            else
                fprintf(output,"\t%i [shape=ellipse width=%i penlabel=\"\"];\n", n->number, node_size);
        }
        state_set childnodes;
        set<int> sinks;
        for(int i = 0; i < aut->alphabet.size(); ++i){
            apta_node* child = n->get_child(i);
            if(child == 0){
                // no output
            } else {
                 if(sink_type(child) != -1){
                     sinks.insert(sink_type(child));
                 } else {
            int node_size = int((float)(l->pos(i) + l->neg(i))/5.0) + 1;
            if (aut->alph_str(i) == "0")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"cyan\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "1")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"cyan\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "2")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "3")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"green\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "4")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "5")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "6")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"red\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "7")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"green\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));
            if (aut->alph_str(i) == "8")
                fprintf(output, "\t\t%i -> %i [penwidth=%i color=\"green\" label=\"%i:%i\"];\n" ,n->number, child->number, node_size, l->pos(i),l->neg(i));

                     //childnodes.insert(child);
                 }
            }
        }
        for(set<int>::iterator it2 = sinks.begin(); it2 != sinks.end(); ++it2){
            int stype = *it2;
            fprintf(output,"\tS%it%i [label=\"sink %i\" shape=box];\n", n->number, stype, stype);
            fprintf(output, "\t\t%i -> S%it%i [label=\"" ,n->number, n->number, stype);
            for(int i = 0; i < aut->alphabet.size(); ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    fprintf(output, " %s [%i:%i]", aut->alph_str(i).c_str(), l->num_pos[i], l->num_neg[i]);
                }
            }
            fprintf(output, "\"];\n");
        }

    }
    fprintf(output,"}\n");
};
