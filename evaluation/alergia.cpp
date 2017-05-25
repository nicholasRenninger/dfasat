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

int EVAL_TYPE = 1;

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

void alergia_data::print_transition_label(iostream& output, int symbol){
    output << num_pos[symbol];
};

void alergia_data::print_state_label(iostream& output){
    output << num_accepting;
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

bool alergia::alergia_consistency(double right_count, double left_count, double right_total, double left_total){
    double bound = (1.0 / sqrt(left_total) + 1.0 / sqrt(right_total));
    bound = bound * sqrt(0.5 * log(2.0 / CHECK_PARAMETER));
    
    double gamma = (left_count / left_total) - (right_count / right_total);

    if(gamma > bound) return false;
    if(-gamma > bound) return false;

    return true;
};

bool alergia::data_consistent(alergia_data* l, alergia_data* r){
    if(EVAL_TYPE == 1){ if(l->accepting_paths + l->num_accepting < STATE_COUNT || r->accepting_paths + l->num_accepting < STATE_COUNT) return true; }
    else if(l->accepting_paths < STATE_COUNT || r->accepting_paths < STATE_COUNT) return true;
    
    double left_count = 0.0;
    double right_count = 0.0;
    
    double left_total = (double)l->accepting_paths;
    double right_total = (double)r->accepting_paths;
    
    if(EVAL_TYPE == 1){
        left_total += (double)l->num_accepting;
        right_total += (double)r->num_accepting;
    }

    double l1_pool = 0.0;
    double r1_pool = 0.0;
    double l2_pool = 0.0;
    double r2_pool = 0.0;
    double matching_right = 0.0;

    for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
        left_count = (*it).second;
        right_count = r->pos((*it).first);
        matching_right += right_count;
        
        if(left_count >= SYMBOL_COUNT && right_count >= SYMBOL_COUNT){
            if(alergia_consistency(right_count, left_count, right_total, left_total) == false){
                inconsistency_found = true; return false;
            }
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
    if(EVAL_TYPE == 1){
        left_count = l->num_accepting;
        right_count = r->num_accepting;
        if(left_count != 0) matching_right += right_count;
        
        if(left_count >= SYMBOL_COUNT && right_count >= SYMBOL_COUNT){
            if(alergia_consistency(right_count, left_count, right_total, left_total) == false){
                inconsistency_found = true; return false;
            }
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
    
    if(left_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        if(alergia_consistency(right_count, left_count, right_total, left_total) == false){
            inconsistency_found = true; return false;
        }
    }
    
    left_count = l2_pool;
    right_count = r2_pool;
    
    if(left_count >= SYMBOL_COUNT || right_count >= SYMBOL_COUNT) {
        if(alergia_consistency(right_count, left_count, right_total, left_total) == false){
            inconsistency_found = true; return false;
        }
    }
    
    return true;
};

/* ALERGIA, consistency based on Hoeffding bound, only uses positive (type=1) data, pools infrequent counts */
bool alergia::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(count_driven::consistent(merger, left, right) == false){ inconsistency_found = true; return false; }
    //if(left->depth != right->depth) {inconsistency_found = true; return false;};
    alergia_data* l = (alergia_data*) left->data;
    alergia_data* r = (alergia_data*) right->data;
    
    return data_consistent(l, r);
};


/* When is an APTA node a sink state?
 * sink states are not considered merge candidates
 *
 * accepting sink = only accept, accept now, accept afterwards
 * rejecting sink = only reject, reject now, reject afterwards 
 * low count sink = frequency smaller than STATE_COUNT */
bool alergia_data::is_low_count_sink(){
    if(EVAL_TYPE == 1) return num_accepting + num_rejecting + accepting_paths + rejecting_paths < SINK_COUNT;
    return accepting_paths + rejecting_paths < SINK_COUNT;
}

bool alergia_data::is_stream_sink(apta_node* node) {

   return node->size < STREAM_COUNT;
}

int alergia_data::sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    if (is_low_count_sink()) return 0;
    if(is_stream_sink(node)) return 2;
    return -1;
};

bool alergia_data::sink_consistent(int type){
    if(!USE_SINKS) return true;
    
    if(type == 0) return is_low_count_sink();
    //if(type == 1) return this->is_stream_sink();
    return true;
};

int alergia::num_sink_types(){
    if(!USE_SINKS) return 0;
    return 2;
};

/*void alergia::print_dot(iostream& output, state_merger* merger){
    apta* aut = merger->aut;
    state_set s  = merger->red_states;
    
    cerr << "size: " << s.size() << endl;
    
    output << "digraph DFA {\n";
    output << "\t" << aut->root->find()->number << " [label=\"root\" shape=box];\n";
    output << "\t\tI -> " << aut->root->find()->number << ";\n";
    for(state_set::iterator it = merger->red_states.begin(); it != merger->red_states.end(); ++it){
        apta_node* n = *it;
        alergia_data* l = (alergia_data*) n->data;

        output << "\t" << n->number << " [shape=ellipse label=\"[" << l->num_accepting << "]\\n[";

        int my_sum = 0;
        for(int i = 0; i < alphabet_size; ++i){
            my_sum += l->pos(i);
        }
        output << " " << my_sum;
        
        output << "]\"];\n";
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
        // should not need sinks
        for(set<int>::iterator it2 = sinks.begin(); it2 != sinks.end(); ++it2){
            int stype = *it2;
            output << "\tS" << n->number << "t" << stype << " [label=\"sink " << stype << "\" shape=box];\n";
            output << "\t\t" << n->number << " -> S" << n->number << "t" << stype << " [label=\"";
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    output << " " << aut->alph_str(i).c_str() << " [" << ((alergia_data*)n->data)->num_pos[i] << ":" << ((alergia_data*)n->data)->num_neg[i] << "]";

                }
            }
            output << "\"];\n";
        }
        // this is what i care about
        for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            output << "\t\t" << n->number << " -> " << child->number << " [label=\"";
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    output << " " << aut->alph_str(i).c_str() << " [" << ((alergia_data*)n->data)->num_pos[i] << ":" << ((alergia_data*)n->data)->num_neg[i] << "]";
                }
            }
            output << "\"];\n";
        }
    }

    // leak workaround
    state_set *state = &merger->get_candidate_states();
    
    s = *state; // merger->get_candidate_states();

    for(state_set::iterator it = s.begin(); it != s.end(); ++it){
        apta_node* n = *it;
        alergia_data* l = reinterpret_cast<alergia_data*>(n->data);
        if(l->num_accepting != 0){
            output << "\t" << n->number << " [shape=doublecircle style=dotted label=\"" << l->num_accepting << ":" << l->num_rejecting << "\\n[" << l->accepting_paths << ":" << l->rejecting_paths << "]\"];\n";
        } else if(l->num_rejecting != 0){
            output << "\t" << n->number << " [shape=Mcircle style=dotted label=\"" << l->num_accepting << ":" << l->num_rejecting << "\\n[" << l->accepting_paths << ":" << l->rejecting_paths << "]\"];\n";
        } else {
            output << "\t" << n->number << " [shape=circle style=dotted label=\"0:0\\n[" << l->accepting_paths << ":" << l->rejecting_paths << "]\"];\n";
        }
        for(child_map::iterator it2 = n->children.begin(); it2 != n->children.end(); ++it2){
            apta_node* child = (*it2).second;
            int symbol = (*it2).first;
            output << "\t\t" << n->number << " -> " << child->number << " [style=dotted label=\"" << aut->alph_str(symbol).c_str() << " [" << l->num_pos[symbol] << ":" << l->num_neg[symbol] << "]\"];\n";
        }
    }

    output << "}\n";

    delete state;
};*/
