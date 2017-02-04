
#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "parameters.h"

state_merger::state_merger()
: context(){
    context.merger = this;
}

state_merger::state_merger(evaluation_function* e, apta* a)
: context(){
    context.merger = this;
    aut = a;
    eval = e;
    eval->initialize(this);
    reset();
}

void state_merger::reset(){
    red_states.clear();
    blue_states.clear();
    red_states.insert(aut->root);
    update();
}

/* iterators for the APTA and merged APTA */
class APTA_iterator {
public:
    apta_node* base;
    apta_node* current;
    
    APTA_iterator(apta_node* start){
        base = start;
        current = start;
    }
    
    void increment() {
        child_map::iterator it;
        if(!current->children.empty()){ current = (*current->children.begin()).second; return; }
        
        it = current->source->children.find(current->label);
        ++it;
        if(it != current->source->children.end()){ current = (*it).second; return; }
        
        while(current->source != base){
            current = current->source;
            it = current->source->children.find(current->label);
            ++it;
            if(it != current->source->children.end()){ current = (*it).second; return; }
        }
        current = 0;
    }
    
    apta_node* operator*() const { return current; }
    APTA_iterator& operator++() { increment(); return *this; }
};

class merged_APTA_iterator {
public:
    apta_node* base;
    apta_node* current;
    
    merged_APTA_iterator(apta_node* start){
        base = start;
        current = start;
    }
    
    void increment() {
        child_map::iterator it;
        for(it = current->children.begin();it != current->children.end(); ++it){
            if((*it).second->representative == 0){
                current = (*it).second;
                return;
            }
        }

        apta_node* source = current;
        while(source != base){
            current = source;
            source = source->source->find();
            it = source->children.find(current->label);
            ++it;
            for(; it != source->children.end(); ++it){
                if((*it).second->representative == 0){
                    current = (*it).second;
                    return;
                }
            }
        }
        current = 0;
    }
    
    apta_node* operator*() const { return current; }
    merged_APTA_iterator& operator++() { increment(); return *this; }
};

/* GET STATE LISTS */
void add_states(apta_node* state, state_set& states){
    if(states.find(state) != states.end()) return;
    states.insert(state);
    for(child_map::iterator it = state->children.begin();it != state->children.end(); ++it){
        apta_node* child = (*it).second;
        if(child != 0) add_states(child, states);
    }
}

void add_merged_states(apta_node* state, state_set& states){
    if(states.find(state) != states.end()) return;
    states.insert(state);
    for(child_map::iterator it = state->children.begin();it != state->children.end(); ++it){
        apta_node* child = (*it).second;
        add_merged_states(child->find(), states);
    }
}

state_set &apta::get_states(apta_node* node){
    state_set* states = new state_set();
    add_states(node, *states);
    return *states;
}

state_set &apta::get_states(){
    state_set* states = new state_set();
    add_states(root, *states);
    return *states;
}

state_set &apta::get_merged_states(apta_node* node){
    state_set* states = new state_set();
    add_states(node->find(), *states);
    return *states;
}

state_set &apta::get_merged_states(){
    state_set* states = new state_set();
    add_merged_states(root->find(), *states);
    return *states;
}

state_set &apta::get_accepting_states(){
    state_set states = get_states();
    state_set* accepting_states = new state_set();
    for(state_set::iterator it = states.begin();it != states.end();++it){
        if((*it)->type == 1) accepting_states->insert(*it);
    }
    return *accepting_states;
}

state_set &apta::get_rejecting_states(){
    state_set states = get_states();
    state_set* rejecting_states = new state_set();
    for(state_set::iterator it = states.begin();it != states.end();++it){
        if((*it)->type != 1) rejecting_states->insert(*it);
    }
    return *rejecting_states;
}

state_set &state_merger::get_candidate_states(){
    state_set states = blue_states;
    state_set* candidate_states = new state_set();
    for(state_set::iterator it = blue_states.begin();it != blue_states.end();++it){
        if(sink_type(*it) == -1)
            add_merged_states((*it)->find(),*candidate_states);
    }
    return *candidate_states;
}

state_set &state_merger::get_sink_states(){
    state_set states = blue_states;
    state_set* sink_states = new state_set();
    for(state_set::iterator it = blue_states.begin();it != blue_states.end();++it){
        if(sink_type(*it) != -1)
            add_merged_states((*it)->find(),*sink_states);
    }
    return *sink_states;
}

// leak workaround
int state_merger::get_final_apta_size(){
    state_set *s = &get_candidate_states();
    int count = s->size();
    delete s;
    return red_states.size() + count;

}

/* FIND/UNION functions */
apta_node* apta_node::get_child(int c){
    apta_node* rep = find();
    if(rep->child(c) != 0){
      return rep->child(c)->find();
    }
    return 0;
}

apta_node* apta_node::find(){
    if(representative == 0)
        return this;

    return representative->find();
}

apta_node* apta_node::find_until(apta_node* node, int i){
    if(undo(i) == node)
        return this;

    if(representative == 0)
        return 0;

    return representative->find_until(node, i);
}

/* recursive state merging routines */

/* standard merge, the process is interupted when an inconsistency is found */
bool state_merger::merge(apta_node* left, apta_node* right){
    bool result = true;
    if(left == 0 || right == 0) return true;
    if(eval->consistent(this, left, right) == false) return false;
    
    if(left->red && RED_FIXED){
        for(child_map::iterator it = right->children.begin();it != right->children.end(); ++it)
            if(left->child((*it).first) == 0 && !eval->sink_consistent((*it).second, 0)) return false;
    }

    left->data->update(right->data);
    eval->update_score(this, left, right);
    right->representative = left;
    left->size += right->size;
    
    for(child_map::iterator it = right->children.begin();it != right->children.end(); ++it){
        int i = (*it).first;
        apta_node* right_child = (*it).second;
        if(left->child(i) == 0){
            left->children[i] = right_child;
        } else {
            apta_node* child = left->children[i]->find();
            apta_node* other_child = right_child->find();

            if(child != other_child){
                other_child->det_undo[i] = right;
                result = merge(child, other_child);
                if(result == false) break;
            }
        }
    }
    return result;
}

/* forced merge, continued even when inconsistent */
void state_merger::merge_force(apta_node* left, apta_node* right){
    if(left == 0 || right == 0) return;

    left->data->update(right->data);
    right->representative = left;
    left->size += right->size;
    
    for(child_map::iterator it = right->children.begin();it != right->children.end(); ++it){
        int i = (*it).first;
        apta_node* right_child = (*it).second;
        if(left->child(i) == 0){
            left->children[i] = right_child;
        } else {
            apta_node* child = left->children[i]->find();
            apta_node* other_child = right_child->find();

            if(child != other_child){
                other_child->det_undo[i] = right;
                merge_force(child, other_child);
            }
        }
    }
}

/* testing merge, no actual merge is performed, only consistency and heuristic are computed */
bool state_merger::merge_test(apta_node* left, apta_node* right){
    bool result = true;
    if(left == 0 || right == 0) return true;
    if(eval->consistent(this, left, right) == false) return false;
    
    if(left->red && RED_FIXED){
        for(child_map::iterator it = right->children.begin(); it != right->children.end(); ++it)
            if(left->child((*it).first) == 0 && !eval->sink_consistent((*it).second, 0)) return false;
    }    
    eval->update_score(this, left, right);
    for(child_map::iterator it = right->children.begin();it != right->children.end(); ++it){
        int i = (*it).first;
        apta_node* right_child = (*it).second;
        if(left->child(i) != 0){
            apta_node* child = left->children[i]->find();
            apta_node* other_child = right_child->find();

            if(child != other_child){
                result = merge_test(child, other_child);
                if(result == false) break;
            }
        }
    }
    return result;
}

/* undo merge, works for both forcing and standard merging */
void state_merger::undo_merge(apta_node* left, apta_node* right){
    //cerr << "undo merge: " << left << " " << right << endl;
    if(left == 0 || right == 0) return;
    if(right->representative != left) return;

    for(child_map::reverse_iterator it = right->children.rbegin();it != right->children.rend(); ++it){
        int i = (*it).first;
        apta_node* right_child = (*it).second;
        if(left->child(i) == right_child){
            left->children.erase(i);
        } else if(left->child(i) != 0){
            apta_node* other_child = right_child->find_until(right, i);
            apta_node* child = right_child->representative;
            if(child != right_child)
                undo_merge(child, right_child);
            right_child->det_undo.erase(i);
        }
    }

    left->data->undo(right->data);
    left->size -= right->size;
    right->representative = 0;
    eval->undo_update(this, left, right);
}

/* update red blue sets after performing a merge */
void state_merger::update(){
    state_set new_red;
    state_set new_blue;
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        apta_node* red = (*it)->find();
        new_red.insert(red);
        red->red = true;
    }
    for(state_set::iterator it = new_red.begin(); it != new_red.end(); ++it){
        apta_node* red = *it;
        for(int i = 0; i < alphabet_size; ++i){
            apta_node* child = red->get_child(i);
            if(child != 0 && new_red.find(child) == new_red.end()) new_blue.insert(child);
        }
    }
    red_states = new_red;
    blue_states = new_blue;
    eval->update(this);
}

/* functions called by state merging algorithms */
apta_node* state_merger::extend_red(){
    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        apta_node* blue = *it;
        bool found = false;
        if(!MERGE_SINKS_DSOLVE && (sink_type(blue) != -1)) continue;

        for(state_set::iterator it2 = red_states.begin(); it2 != red_states.end(); ++it2){
            apta_node* red = *it2;
            
            int score = testmerge(red, blue);
            if(score != -1) found = true;
        }
        
        if(found == false){
            blue_states.erase(blue);
            red_states.insert(blue);
            blue->red = true;

            for(int i = 0; i < alphabet_size; ++i){
                apta_node* child = blue->get_child(i);
                if(child != 0) blue_states.insert(child);
            }
            return blue;
        }
    }
    return 0;
}

void state_merger::extend(apta_node* blue){
    blue_states.erase(blue);
    red_states.insert(blue);
    blue->red = true;

    for(child_map::iterator it = blue->children.begin(); it != blue->children.end(); ++it){
        apta_node* child = (*it).second;
        blue_states.insert(child);
    }
}

void state_merger::undo_extend(apta_node* blue){
    blue_states.insert(blue);
    red_states.erase(blue);
    blue->red = false;

    for(child_map::iterator it = blue->children.begin(); it != blue->children.end(); ++it){
        apta_node* child = (*it).second;
        blue_states.erase(child);
    }
}

bool state_merger::perform_merge(apta_node* left, apta_node* right){
    /*
    eval->reset(this);
    bool result = merge(left->find(), right->find());
    if(result == false || eval->compute_consistency(this, left, right) == false){
        undo_merge(left->find(),right->find());
        return false;
    }
    */
    merge_force(left, right);
    update();
    return true;
}

bool state_merger::undo_perform_merge(apta_node* left, apta_node* right){
    /*
    eval->reset(this);
    bool result = merge(left->find(), right->find());
    if(result == false || eval->compute_consistency(this, left, right) == false){
        undo_merge(left->find(),right->find());
        return false;
    }
    */
    undo_merge(left, right);
    update();
    return true;
}

int state_merger::testmerge(apta_node* left, apta_node* right){
    eval->reset(this);
    int result = -1;
    bool merge_result = false;
    
    if(eval->compute_before_merge) result = eval->compute_score(this, left, right);
    
    if(MERGE_WHEN_TESTING){
        merge_result = merge(left,right);
    } else {
        merge_result = merge_test(left,right);
    }
    
    if(merge_result && !eval->compute_before_merge) result = eval->compute_score(this, left, right);

    if((merge_result && eval->compute_consistency(this, left, right) == false) ||  result < LOWER_BOUND) result = -1;
    
    if(MERGE_WHEN_TESTING) undo_merge(left,right);
    
    if(merge_result == false) return -1;
    return result;
}

int state_merger::test_local_merge(apta_node* left, apta_node* right){
    if(left == 0 || right == 0) return -1;
    eval->reset(this);
    if(eval->consistent(this, left, right) == false) return -1;
    
    if(left->red && RED_FIXED){
        for(child_map::iterator it = right->children.begin(); it != right->children.end(); ++it)
            if(left->child((*it).first) == 0 && !eval->sink_consistent((*it).second, 0)) return -1;
    }
    
    if(!eval->compute_before_merge)
        eval->update_score(this, left, right);
    return eval->compute_score(this, left, right);
}

merge_map* state_merger::get_possible_merges(){
    // sicco: I added state_set to make it compile
    state_set states = aut->get_merged_states();
    for(merged_APTA_iterator Ait = merged_APTA_iterator(aut->root); *Ait != 0; ++Ait){
        if(states.find(*Ait) == states.end()) cerr << "mAit element not found " << (*Ait)->source->number << " " << (*Ait)->label << " " << (*Ait)->number << endl;
        else states.erase(*Ait);
    }
    if(!states.empty()) cerr << "mAit set not empty" << endl;

    eval->reset(this);
    merge_map* mset = new merge_map();
    
    /*apta_node* max_blue = 0;
    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        if(!MERGE_SINKS_DSOLVE && (sink_type(*it) != -1)) continue;
        if(max_blue == 0 || max_blue->size < (*it)->size)
            max_blue = *it;
    }*/
    

    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        apta_node* blue = *(it);
        if(!MERGE_SINKS_DSOLVE && (sink_type(blue) != -1)) continue;
        
        //if(*it != max_blue) continue;


        for(state_set::iterator it2 = red_states.begin(); it2 != red_states.end(); ++it2){
            apta_node* red = *it2;

            int score = testmerge(red,blue);
            if(score > -1){
                mset->insert(pair<int, pair<apta_node*, apta_node*> >(score, pair<apta_node*, apta_node*>(red, blue)));
            }
        }
        
        if(MERGE_BLUE_BLUE){
            for(state_set::iterator it2 = blue_states.begin(); it2 != blue_states.end(); ++it2){
                apta_node* blue2 = *it2;
                
                if(blue == blue2) continue;

                int score = testmerge(blue2,blue);
                if(score > -1){
                    mset->insert(pair<int, pair<apta_node*, apta_node*> >(score, pair<apta_node*, apta_node*>(blue2, blue)));
                }
            }
        }
        
        if(MERGE_MOST_VISITED) break;
    }
    return mset;
}

merge_pair* state_merger::get_best_merge(){
    eval->reset(this);
    merge_pair* best = new merge_pair(0,0);
    int result = -1;

    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        apta_node* blue = *(it);
        if(!MERGE_SINKS_DSOLVE && (sink_type(blue) != -1)) continue;
        
        for(state_set::iterator it2 = red_states.begin(); it2 != red_states.end(); ++it2){
            apta_node* red = *it2;
            
            int score = testmerge(red,blue);
            if(score > -1 && score > result){
                best->first = red;
                best->second = blue;
                result = score;
            }
        }
        
        if(MERGE_BLUE_BLUE){
            for(state_set::iterator it2 = blue_states.begin(); it2 != blue_states.end(); ++it2){
                apta_node* blue2 = *it2;
                
                if(blue == blue2) continue;

                int score = testmerge(blue2,blue);
                if(score > -1 && score > result){
                    best->first = blue2;
                    best->second = blue;
                    result = score;
                }
            }
        }
        
        if(MERGE_MOST_VISITED) break;
    }
    return best;
}


/* input function 	        *
 * pass along to  eval fct      */

// streaming mode methods
void state_merger::init_apta(string data) {
   eval->init(data, this);
}

void state_merger::advance_apta(string data, state_merger* merger) {
   eval->add_sample(data, merger);
}

// batch mode methods
void state_merger::read_apta(istream &input_stream){
    eval->read_file(input_stream, this);
}

void state_merger::read_apta(string dfa_file){
    ifstream input_stream(dfa_file);
    read_apta(input_stream);
    input_stream.close();
}

void state_merger::read_apta(vector<string> dfa_data){
    stringstream input_stream;
    for (int i = 0; i < dfa_data.size(); i++) {
        input_stream << dfa_data[i]; 
    }
    //eval->read_file(input_stream, this);
    read_apta(input_stream);
}


/* output functions */
void state_merger::todot(){
    stringstream dot_output_buf;
    cout << this;
    eval->print_dot(dot_output_buf, this);
    dot_output = dot_output_buf.str();
}

void state_merger::print_dot(FILE* output)
{
    fprintf(output, "%s", dot_output.c_str());
}

int state_merger::sink_type(apta_node* node){
    return eval->sink_type(node);
};

bool state_merger::sink_consistent(apta_node* node, int type){
    return eval->sink_consistent(node, type);
};

int state_merger::num_sink_types(){
    return eval->num_sink_types();
};

int state_merger::compute_global_score(){
    return get_final_apta_size();
};

