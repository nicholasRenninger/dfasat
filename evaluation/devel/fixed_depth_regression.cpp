#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "depth-driven.h"

REGISTER_DEF_DATATYPE(fixed_depth_mse_data);
REGISTER_DEF_TYPE(fixed_depth_mse_error);

bool is_single_event_sink(apta_node* node, int event){
    node = node->find();
    if(node->children.size() == 0) return true;
    if(node->children.size() != 1 || node->child(event) == 0) return false;
    apta_node* child = node->child(event);
    return is_single_event_sink(child, event);
}

bool is_single_event_sink(apta_node* node){
    node = node->find();
    if(node->children.size() == 0) return true;
    if(node->children.size() != 1) return false;
    int event = (*node->children.begin()).first;
    apta_node* child = node->child(event);
    return is_single_event_sink(child, event);
}

bool is_all_the_same_sink(apta_node* node){
    node = node->find();
    if(node->children.size() == 0) return true;
    if(node->children.size() != 1) return false;
    for(child_map::iterator it = node->children.begin(); it != node->children.end(); ++it){
        int event = (*it).first;
        apta_node* child = node->child(event);
        if(is_all_the_same_sink(child) == false) return false;
    }
    return true;
}

bool is_zero_occ_sink(apta_node* node){
    node = node->find();
    if(node->children.size() == 0) return true;
    //if(node->children.size() != 1) return false;
    if(node->occs.size() > 0) return false;
    int event = (*node->children.begin()).first;
    apta_node* child = node->child(event);
    return is_zero_occ_sink(child);
}

int get_event_type(apta_node* node){
    node = node->find();
    return (*node->children.begin()).first;
}

bool fixed_depth_mse_error::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(left->depth != right->depth){ inconsistency_found = true; return false; }
    if(mse_error::consistent(merger,left,right) == false) return false;
    return true;
};

void fixed_depth_mse_error::print_dot(FILE* output, state_merger* merger){
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
