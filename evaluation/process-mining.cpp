#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "process-mining.h"
#include "parameters.h"

REGISTER_DEF_DATATYPE(process_data);
REGISTER_DEF_TYPE(process_mining);

void process_data::print_state_label(iostream& output){
    for(set<int>::iterator it = done_tasks.begin(); it != done_tasks.end(); ++it){
        output << *it << " ";
    }
    output << "\n";
    for(set<int>::iterator it = future_tasks.begin(); it != future_tasks.end(); ++it){
        output << *it << " ";
    }
    output << "\n";
};

void process_data::update(evaluation_data* right){
    overlap_data::update(right);

    process_data* r = (process_data*)right;
    
    set<int>::iterator it  = future_tasks.begin();
    set<int>::iterator it2 = r->future_tasks.begin();
    r->undo_info.clear();
    
    while(it != future_tasks.end() && it2 != r->future_tasks.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else if(*it < *it2){
            it++;
        } else {
            r->undo_info.insert(*it2);
            it2++;
        }
    }
    while(it2 != r->future_tasks.end()){
        r->undo_info.insert(*it2);
        it2++;
    }
    
    for(it = r->undo_info.begin(); it != r->undo_info.end(); ++it){
        future_tasks.insert(*it);
    }

    it  = done_tasks.begin();
    it2 = r->done_tasks.begin();
    r->undo_dinfo.clear();
    
    while(it != done_tasks.end() && it2 != r->done_tasks.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else if(*it < *it2){
            it++;
        } else {
            r->undo_dinfo.insert(*it2);
            it2++;
        }
    }
    while(it2 != r->done_tasks.end()){
        r->undo_dinfo.insert(*it2);
        it2++;
    }
    
    for(it = r->undo_dinfo.begin(); it != r->undo_dinfo.end(); ++it){
        done_tasks.insert(*it);
    }
};

void process_data::undo(evaluation_data* right){
    overlap_data::undo(right);
    
    process_data* r = (process_data*)right;
    
    for(set<int>::iterator it = r->undo_info.begin(); it != r->undo_info.end(); ++it){
        future_tasks.erase(*it);
    }
    for(set<int>::iterator it = r->undo_dinfo.begin(); it != r->undo_dinfo.end(); ++it){
        done_tasks.erase(*it);
    }
};

bool process_mining::consistent(state_merger *merger, apta_node* left, apta_node* right){
    //return overlap_driven::consistent(merger, left, right);

    process_data* l = (process_data*)left->data;
    process_data* r = (process_data*)right->data;

    //if(l->accepting_paths >= STATE_COUNT && r->accepting_paths >= STATE_COUNT)
    //    return overlap_driven::consistent(merger, left, right);
    
    if(l->accepting_paths + l->num_accepting > STATE_COUNT){
        for(num_map::iterator it = r->num_pos.begin(); it != r->num_pos.end(); ++it){
            if(l->pos((*it).first) == 0){
                inconsistency_found = true;
                return false;
            }
        }
    }
    if(r->accepting_paths + r->num_accepting > STATE_COUNT){
        for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
            if(r->pos((*it).first) == 0){
                inconsistency_found = true;
                return false;
            }
        }
    }
    return true;
    
    set<int>::iterator it  = l->done_tasks.begin();
    set<int>::iterator it2 = r->done_tasks.begin();
    
    while(it != l->done_tasks.end() && it2 != r->done_tasks.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else {
            return false;
        }
    }
    if(it != l->done_tasks.end() || it2 != r->done_tasks.end()){
        return false;
    }
    return true;
};

void process_mining::update_score(state_merger *merger, apta_node* left, apta_node* right){
    return overlap_driven::update_score(merger, left, right);
};

bool process_mining::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
    process_data* l = (process_data*)left->data;
    process_data* r = (process_data*)right->data;

    //return overlap_driven::compute_consistency(merger, left, right);

    //if(l->accepting_paths >= STATE_COUNT && r->accepting_paths >= STATE_COUNT)
    //    return overlap_driven::compute_consistency(merger, left, right);
    
    set<int> diff;
    for(set<int>::iterator it = r->future_tasks.begin(); it != r->future_tasks.end(); ++it){
        if(l->future_tasks.find(*it) == l->future_tasks.end()) return false;//diff.insert(*it);//return false;
    }
    
    if(diff.empty() == false){
    for(set<int>::iterator it = l->done_tasks.begin(); it != l->done_tasks.end(); ++it){
        if(r->done_tasks.find(*it) == r->done_tasks.end() && diff.find(*it) == diff.end()) return false;
    }}
    diff.clear();
    for(set<int>::iterator it = l->future_tasks.begin(); it != l->future_tasks.end(); ++it){
        if(r->future_tasks.find(*it) == r->future_tasks.end()) return false;//diff.insert(*it);//return false;
    }
    if(diff.empty() == false){
    for(set<int>::iterator it = r->done_tasks.begin(); it != r->done_tasks.end(); ++it){
        if(l->done_tasks.find(*it) == l->done_tasks.end() && diff.find(*it) == diff.end()) return false;
    }}
    return overlap_driven::compute_consistency(merger, left, right);

    for(set<int>::iterator it = r->done_tasks.begin(); it != r->done_tasks.end(); ++it){
        if(l->done_tasks.find(*it) == l->done_tasks.end()) return false;
    }

    for(num_map::iterator it = r->num_pos.begin(); it != r->num_pos.end(); ++it){
        if(l->pos((*it).first) == 0) return false;
    }
    return overlap_driven::compute_consistency(merger, left, right);
};

int process_mining::compute_score(state_merger* m, apta_node* left, apta_node* right){
    process_data* l = (process_data*)left->data;
    process_data* r = (process_data*)right->data;
    
    return overlap_driven::compute_score(m, left, right);

    set<int>::iterator it  = l->future_tasks.begin();
    set<int>::iterator it2 = r->future_tasks.begin();
    
    while(it != l->future_tasks.end() && it2 != r->future_tasks.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else {
            return overlap_driven::compute_score(m, left, right);
        }
    }
    if(it != l->future_tasks.end() || it2 != r->future_tasks.end()){
        return overlap_driven::compute_score(m, left, right);
    }
    return LOWER_BOUND + overlap_driven::compute_score(m, left, right);
};

void process_mining::initialize(state_merger* m){
    for(merged_APTA_iterator it = merged_APTA_iterator(m->aut->root); *it != 0; ++it){
        apta_node* n = *it;
        apta_node* p = n;
        process_data* data = (process_data*)n->data;
        while(p->source != 0){
            data->done_tasks.insert(p->label);
            p = p->source;
        }
    }

    for(merged_APTA_iterator it = merged_APTA_iterator(m->aut->root); *it != 0; ++it){
        apta_node* n = *it;
        apta_node* p = n;
        while(p->source != 0){
            process_data* data = (process_data*)p->source->data;
            data->future_tasks.insert(n->label);
            p = p->source;
        }
    }
};
