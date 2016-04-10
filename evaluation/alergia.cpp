#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "state_merger.h"
#include "evaluate.h"
#include "depth-driven.h"
#include "alergia.h"

#include "parameters.h"

REGISTER_DEF_DATATYPE(alergia_data);
REGISTER_DEF_TYPE(alergia);

alergia_data::alergia_data(){
    num_pos = num_map();
    num_neg = num_map();
};

void alergia_data::read_from(int type, int index, int length, int symbol, string data){
    count_data::read_from(type, index, length, symbol, data);
    if(type == 1){
        num_pos[symbol] = pos(symbol) + 1;
    } else {
        num_neg[symbol] = neg(symbol) + 1;
    }
};

void alergia_data::update(evaluation_data* right){
    count_data::update(right);
    alergia_data* other = (alergia_data*)right;
    for(num_map::iterator it = other->num_pos.begin();it != other->num_pos.end(); ++it){
        num_pos[(*it).first] = pos((*it).first) + (*it).second;
    }
    for(num_map::iterator it = other->num_neg.begin();it != other->num_neg.end(); ++it){
        num_neg[(*it).first] = neg((*it).first) + (*it).second;
    }
};

void alergia_data::undo(evaluation_data* right){
    count_data::undo(right);
    alergia_data* other = (alergia_data*)right;
    for(num_map::iterator it = other->num_pos.begin();it != other->num_pos.end(); ++it){
        num_pos[(*it).first] = pos((*it).first) - (*it).second;
    }
    for(num_map::iterator it = other->num_neg.begin();it != other->num_neg.end(); ++it){
        num_neg[(*it).first] = neg((*it).first) - (*it).second;
    }
};

/* ALERGIA, consistency based on Hoeffding bound, only uses positive (type=1) data, pools infrequent counts */
bool alergia::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(count_driven::consistent(merger, left, right) == false){ inconsistency_found = true; return false; }
    alergia_data* l = (alergia_data*) left->data;
    alergia_data* r = (alergia_data*) right->data;

    if(l->accepting_paths < STATE_COUNT || r->accepting_paths < STATE_COUNT) return true;

    double bound = sqrt(1.0 / (double)l->accepting_paths) + sqrt(1.0 / (double)r->accepting_paths);
    bound = bound * sqrt(0.5 * log(2.0 / CHECK_PARAMETER));
    
    int left_count = 0;
    int right_count  = 0;
    
    int l1_pool = 0;
    int r1_pool = 0;
    int l2_pool = 0;
    int r2_pool = 0;
    int matching_right = 0;

    for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
        left_count = (*it).second;
        right_count = r->pos((*it).first);
        matching_right += right_count;
        
        if(right_count >= SYMBOL_COUNT && right_count >= SYMBOL_COUNT) {
            double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
            if(gamma > bound){ inconsistency_found = true; return false; }
            if(gamma < -bound){ inconsistency_found = true; return false; }
        }

        if(right_count < SYMBOL_COUNT){
            l1_pool += left_count;
            r1_pool += right_count;
        }
        if(left_count < SYMBOL_COUNT) {
            l2_pool += left_count;
            r2_pool += right_count;
        }
    }
    r2_pool += r->accepting_paths - matching_right;
    
    left_count = l1_pool;
    right_count = r1_pool;
    
    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
        if(gamma > bound){ inconsistency_found = true; return false; }
        if(gamma < -bound){ inconsistency_found = true; return false; }
    }
    
    left_count = l2_pool;
    right_count = r2_pool;
    
    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
        if(gamma > bound){ inconsistency_found = true; return false; }
        if(gamma < -bound){ inconsistency_found = true; return false; }
    }
    
    left_count = l->num_accepting;
    right_count = r->num_accepting;

    if(right_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        double gamma = ((double)left_count / (double)l->accepting_paths) - ((double)right_count / (double)r->accepting_paths);
        if(gamma > bound){ inconsistency_found = true; return false; }
        if(gamma < -bound){ inconsistency_found = true; return false; }
    }
    
    return true;
};

void alergia::print_dot(FILE* output, state_merger* merger){
    apta* aut = merger->aut;
    state_set s  = merger->red_states;
    
    cerr << "size: " << s.size() << endl;
    
    fprintf(output,"digraph DFA {\n");
    fprintf(output,"\t%i [label=\"root\" shape=box];\n", aut->root->find()->number);
    fprintf(output,"\t\tI -> %i;\n", aut->root->find()->number);
    for(state_set::iterator it = s.begin(); it != s.end(); ++it){
        apta_node* n = *it;
        alergia_data* l = reinterpret_cast<alergia_data*>(n->data);
        if(l->num_accepting != 0){
            fprintf(output,"\t%i [shape=doublecircle label=\"%i:%i\\n[%i:%i]\"];\n", n->number, l->num_accepting, l->num_rejecting, l->accepting_paths, l->rejecting_paths);
        } else if(l->num_rejecting != 0){
            fprintf(output,"\t%i [shape=Mcircle label=\"%i:%i\\n[%i:%i]\"];\n", n->number, l->num_accepting, l->num_rejecting, l->accepting_paths, l->rejecting_paths);
        } else {
            fprintf(output,"\t%i [shape=circle label=\"0:0\\n[%i:%i]\"];\n", n->number, l->accepting_paths, l->rejecting_paths);
        }
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
            fprintf(output, "\t\t%i -> %i [label=\"" ,n->number, child->number);
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    fprintf(output, " %s [%i:%i]", aut->alph_str(i).c_str(), l->num_pos[i], l->num_neg[i]);
                }
            }
            fprintf(output, "\"];\n");
        }
    }

    s = merger->get_candidate_states();
    for(state_set::iterator it = s.begin(); it != s.end(); ++it){
        apta_node* n = *it;
        alergia_data* l = reinterpret_cast<alergia_data*>(n->data);
        if(l->num_accepting != 0){
            fprintf(output,"\t%i [shape=doublecircle style=dotted label=\"%i:%i\\n[%i:%i]\"];\n", n->number, l->num_accepting, l->num_rejecting, l->accepting_paths, l->rejecting_paths);
        } else if(l->num_rejecting != 0){
            fprintf(output,"\t%i [shape=Mcircle style=dotted label=\"%i:%i\\n[%i:%i]\"];\n", n->number, l->num_accepting, l->num_rejecting, l->accepting_paths, l->rejecting_paths);
        } else {
            fprintf(output,"\t%i [shape=circle style=dotted label=\"0:0\\n[%i:%i]\"];\n", n->number, l->accepting_paths, l->rejecting_paths);
        }
        for(child_map::iterator it2 = n->children.begin(); it2 != n->children.end(); ++it2){
            apta_node* child = (*it2).second;
            int symbol = (*it2).first;
            fprintf(output, "\t\t%i -> %i [style=dotted label=\"%s [%i:%i]\"];\n" ,n->number, child->number, aut->alph_str(symbol).c_str(), l->num_pos[symbol], l->num_neg[symbol]);
        }
    }
    fprintf(output,"}\n");
};
