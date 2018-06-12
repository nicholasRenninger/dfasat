#include "state_merger.h"
#include "conflict-driven.h"
#include "evaluation_factory.h"
#include "num_count.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

REGISTER_DEF_DATATYPE(conflict-edsm_data);
REGISTER_DEF_TYPE(conflict-edsm_driven);

void conflict_data::update(evaluation_data* right){
    count_data::update(right);

    conflict_data* r = (conflict_data*)right;
    
    set<apta_node*>::iterator it  = conflicts.begin();
    set<apta_node*>::iterator it2 = r->conflicts.begin();
    r->undo_info.clear();
    
    while(it != conflicts.end() && it2 != r->conflicts.end()){
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
    while(it2 != r->conflicts.end()){
        r->undo_info.insert(*it2);
        it2++;
    }
    
    for(it = r->undo_info.begin(); it != r->undo_info.end(); ++it){
        conflicts.insert(*it);
    }
};

void conflict_data::undo(evaluation_data* right){
    count_data::undo(right);
    
    conflict_data* r = (conflict_data*)right;
    
    for(set<apta_node*>::iterator it = r->undo_info.begin(); it != r->undo_info.end(); ++it){
        conflicts.erase(*it);
    }
};

void conflict_driven::update_score(state_merger *merger, apta_node* left, apta_node* right){
    conflict_data* r = (conflict_data*)right->data;
    int val = r->conflicts.size() - r->undo_info.size();
    score = score + val;
};

int conflict_driven::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    //conflict_data* r = (conflict_data*)right->data;
    //int val = r->conflicts.size() - r->undo_info.size();
    //cerr << r->conflicts.size() << endl;
    if (score > 0) return score;
    return 0;
};

void conflict_driven::reset(state_merger *merger){
    score = 0;
    count_driven::reset(merger);
};

struct conflict_compare
{
    bool operator()(apta_node* left, apta_node* right) const
    {
        conflict_data* l = (conflict_data*)left->data;
        conflict_data* r = (conflict_data*)right->data;
        if(l->conflicts.size() > r->conflicts.size())
            return 1;
        if(l->conflicts.size() < r->conflicts.size())
            return 0;
        return left->number < right->number;
    }
};

void keep_conflicts(set<apta_node*>* s, conflict_data* d){
    set<apta_node*>::iterator it  = s->begin();
    set<apta_node*>::iterator it2 = d->conflicts.begin();
    
    while(it != s->end() && it2 != d->conflicts.end()){
        if(*it == *it2){
            it++;
            it2++;
        } else if(*it < *it2){
            it = s->erase(it);
        } else {
            it2++;
        }
    }
    while(it != s->end()){
        it = s->erase(it);
    }
};

void remove_conflicts(set<apta_node*>* s, conflict_data* d){
    set<apta_node*>::iterator it  = s->begin();
    set<apta_node*>::iterator it2 = d->conflicts.begin();
    
    while(it != s->end() && it2 != d->conflicts.end()){
        if(*it == *it2){
            it = s->erase(it);
            it2++;
        } else if(*it < *it2){
            it++;
        } else {
            it2++;
        }
    }
};

bool full_overlap(conflict_data* d, conflict_data* d2){
    set<apta_node*>::iterator it  = d->conflicts.begin();
    set<apta_node*>::iterator it2 = d2->conflicts.begin();
    
    while(it != d->conflicts.end() && it2 != d2->conflicts.end()){
        if(*it == *it2){
            ++it;
            ++it2;
        } else if(*it < *it2){
            return false;
        } else {
            ++it2;
        }
    }
    return true;
};

int count_overlap(set<apta_node*>* s, conflict_data* d){
    set<apta_node*>::iterator it  = s->begin();
    set<apta_node*>::iterator it2 = d->conflicts.begin();
    
    int count = 0;
    
    while(it != s->end() && it2 != d->conflicts.end()){
        if(*it == *it2){
            count++;
            it++;
            it2++;
        } else if(*it < *it2){
            it++;
        } else {
            it2++;
        }
    }
    
    return count;
};

void conflict_driven::initialize(state_merger *merger){
    count_driven::initialize(merger);
    compute_before_merge = false;
    
    for(merged_APTA_iterator it = merged_APTA_iterator(merger->aut->root); *it != 0; ++it){
        apta_node* n1 = *it;

        for(merged_APTA_iterator it2 = it; *it2 != 0; ++it2){
            if(*it == *it2) continue;
            apta_node* n2 = *it2;
            
            reset(merger);
            if(merger->merge_test(n1,n2) == false){
                conflict_data* d1 = (conflict_data*)n1->data;
                conflict_data* d2 = (conflict_data*)n2->data;
                d1->conflicts.insert(n2);
                d2->conflicts.insert(n1);
            }
        }
    }
    
    return;
    
    set<apta_node*, conflict_compare> all_nodes;
    for(merged_APTA_iterator it = merged_APTA_iterator(merger->aut->root); *it != 0; ++it){
        conflict_data* d = (conflict_data*)(*it)->data;
        if(d->conflicts.size() < 50) continue;
        all_nodes.insert(*it);
    }
    
    for(set<apta_node*, conflict_compare>::reverse_iterator it = all_nodes.rbegin(); it != all_nodes.rend(); ++it){
        apta_node* n = *it;
        conflict_data* d = (conflict_data*)n->data;
        if(d->conflicts.size() < 50) continue;
        set<apta_node*> bip_nodes;
        bip_nodes.insert(n);
        
        set<apta_node*, conflict_compare>::reverse_iterator it2 = it;
        for(++it2; it2 != all_nodes.rend(); ++it2){
            apta_node* n2 = *it2;
            conflict_data* d2 = (conflict_data*)n2->data;
            if(d->conflicts.find(n2) == d->conflicts.end() && full_overlap(d, d2)){
                bip_nodes.insert(n2);
            }
        }
        
        cerr << "size: " << bip_nodes.size() << " conflicts: " << d->conflicts.size() << endl;

        if(bip_nodes.size() > 50){
            cerr << "removing ";
            set<apta_node*> bip2_nodes = d->conflicts;
            for(set<apta_node*>::iterator itl = bip_nodes.begin(); itl != bip_nodes.end(); ++itl){
                apta_node* l = *itl;
                conflict_data* dl = (conflict_data*)l->data;
                for(set<apta_node*>::iterator itr = bip2_nodes.begin(); itr != bip2_nodes.end(); ++itr){
                    apta_node* r = *itr;
                    conflict_data* dr = (conflict_data*)r->data;
                    set<apta_node*>::iterator itn = dl->conflicts.find(r);
                    if(itn == dl->conflicts.end()) cerr << "not found" << endl;
                    else dl->conflicts.erase(itn);
                    itn = dr->conflicts.find(l);
                    if(itn == dr->conflicts.end()) cerr << "not found" << endl;
                    else dr->conflicts.erase(itn);
                }
                cerr << l->number << " ";
            }
            cerr << endl;
        }
    }
    
    /*
    while(true){
        set<apta_node*> left;
        set<apta_node*> right;
        set<apta_node*, conflict_compare> all_nodes;
    
        for(merged_APTA_iterator it = merged_APTA_iterator(merger->aut->root); *it != 0; ++it){
            conflict_data* d = (conflict_data*)(*it)->data;
            if(d->conflicts.size() < 50) continue;
            //left.insert(*it);
            //right.insert(*it);
            all_nodes.insert(*it);
        }
    
        for(set<apta_node*, conflict_compare>::reverse_iterator it = all_nodes.rbegin(); it != all_nodes.rend(); ++it){
            apta_node* n = *it;
            conflict_data* d = (conflict_data*)n->data;
            set<apta_node*>::iterator itl = left.find(*it);
            set<apta_node*>::iterator itr = right.find(*it);
            if(itl != left.end()){
                cerr << "l" << count_overlap(&right, ((conflict_data*)n->data)) << endl;
                //if(count_overlap(&right, ((conflict_data*)n->data)) > 0.1 * right.size()){
                if(full_overlap(&right, ((conflict_data*)n->data))){
                    remove_conflicts(&left, ((conflict_data*)n->data));
                    //keep_conflicts(&right, ((conflict_data*)n->data));
                } else {
                    left.erase(itl);
                }
            } else if(itr != right.end()){
                cerr << "r" << count_overlap(&left, ((conflict_data*)n->data)) << endl;
                //if(count_overlap(&left, ((conflict_data*)n->data)) > 0.1 * left.size()){
                if(full_overlap(&left, ((conflict_data*)n->data))){
                    remove_conflicts(&right, ((conflict_data*)n->data));
                    //keep_conflicts(&left, ((conflict_data*)n->data));
                } else {
                    right.erase(itr);
                }
            }
        }
        
        cerr << "erasing: " << left.size() << " " << right.size() << endl;
        
        for(set<apta_node*>::iterator it = left.begin(); it != left.end(); ++it){
            for(set<apta_node*>::iterator it2 = right.begin(); it2 != right.end(); ++it2){
                apta_node* l = *it;
                conflict_data* dl = (conflict_data*)l->data;
                apta_node* r = *it2;
                conflict_data* dr = (conflict_data*)r->data;
                set<apta_node*>::iterator itn = dl->conflicts.find(r);
                if(itn == dl->conflicts.end()) cerr << "not found" << endl;
                else dl->conflicts.erase(itn);
                itn = dr->conflicts.find(l);
                if(itn == dr->conflicts.end()) cerr << "not found" << endl;
                else dr->conflicts.erase(itn);
            }
        }
    }
    */
    
    
};

