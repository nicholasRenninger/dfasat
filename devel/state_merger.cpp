
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

int alphabet_size = 0;
bool MERGE_SINKS_DSOLVE = 0;

/* constructors and destructors */
apta::apta(ifstream &input_stream){
    int num_words;
    int node_number = 1;
    input_stream >> num_words >> alphabet_size;
    root = new apta_node();
    
    for(int line = 0; line < num_words; line++){
        apta_node* node = new apta_node*()
        node->number = node_number++;
        
        int positive, length;
        input_stream >> positive >> length;
        
        for(int index = 0; index < length; index++){
            string tuple;
            input_stream >> tuple;

            stringstream lineStream;
            lineStream.str(tuple);
            
            string cell;
            getline(lineStream,cell,':');
            int event = stoi(cell);
            vector<int> attributes;
            lineStream.str(cell);
            while(std::getline(lineStream,cell,',')){
                attributes.push_back(stoi(cell));
            }
            
            node->edat.initialize(positive, index, event, attributes);

            apta_node* next_node = new apta_node();
            next_node->number = node_number++;

            node->add_child(event, attributes, next_node);
            next_node->source = node;
            next_node->label  = c;
            next_node->number = node_number++;
            next_node->depth = depth;
            
            if(positive){
                node->num_pos[c] = 1;
                node->accepting_paths++;
            } else {
                node->num_neg[c] = 1;
                node->rejecting_paths++;
            }
            } else {
                if(positive){
                    node->num_pos[c] = node->pos(c) + 1;
                    node->accepting_paths++;
                } else {
                    node->num_neg[c] = node->neg(c) + 1;
                    node->rejecting_paths++;
                }
            }
            node = node->child(c);
        }
        if(depth > max_depth)
            max_depth = depth;
        if(positive) node->num_accepting++;
        else node->num_rejecting++;
    }
}

apta::~apta(){
    delete root;
}

string apta::alph_str(int i){
    std::ostringstream oss;
    oss << "<";
    for(int j = 0; j < alphabet[i].size(); ++j){
        oss << alphabet[i][j];
        if(j < alphabet[i].size()-1)
            oss << ",";
    }
    oss << ">";
    return oss.str();
}

apta_node::apta_node(){
    source = 0;
    representative = 0;

    children = child_map();
    det_undo = child_map();
    num_pos = num_map();
    num_neg = num_map();

    num_accepting = 0;
    num_rejecting = 0;
    accepting_paths = 0;
    rejecting_paths = 0;
    label = 0;
    number = 0;
    satnumber = 0;
    colour = 0;
    size = 1;
    depth = 0;
    old_depth = 0;
}

apta_node::~apta_node(){
    for(child_map::iterator it = children.begin();it != children.end(); ++it){
        delete (*it).second;
    }
}

state_merger::state_merger(){
}

state_merger::state_merger(evaluation_function* e, apta* a){
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
        if(child != 0) add_states(child->find(), states);
    }
}

state_set &apta::get_states(){
    state_set* states = new state_set();
    add_states(root->find(), *states);
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
        if((*it)->num_accepting != 0) accepting_states->insert(*it);
    }
    return *accepting_states;
}

state_set &apta::get_rejecting_states(){
    state_set states = get_states();
    state_set* rejecting_states = new state_set();
    for(state_set::iterator it = states.begin();it != states.end();++it){
        if((*it)->num_rejecting != 0) rejecting_states->insert(*it);
    }
    return *rejecting_states;
}

state_set &state_merger::get_candidate_states(){
    state_set states = blue_states;
    state_set* candidate_states = new state_set();
    for(state_set::iterator it = blue_states.begin();it != blue_states.end();++it){
        if(sink_type(*it) == -1)
            add_states(*it,*candidate_states);
    }
    return *candidate_states;
}

state_set &state_merger::get_sink_states(){
    state_set states = blue_states;
    state_set* sink_states = new state_set();
    for(state_set::iterator it = blue_states.begin();it != blue_states.end();++it){
        if(sink_type(*it) != -1)
            add_states(*it,*sink_states);
    }
    return *sink_states;
}

int state_merger::get_final_apta_size(){
    return red_states.size() + get_candidate_states().size();
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

/* state merging */
void state_merger::merge(apta_node* left, apta_node* right){
    if(left == 0 || right == 0 || right == left) return;
    
    bool consistent = eval->consistent(this, left,right);
    if(consistent) eval->update_score(this, left,right);
    
    right->representative = left;
    
    left->size          += right->size;
    left->num_accepting += right->num_accepting;
    left->num_rejecting += right->num_rejecting;
    left->accepting_paths += right->accepting_paths;
    left->rejecting_paths += right->rejecting_paths;
    
    //left->old_depth = left->depth;
    //left->depth = min(left->depth, right->depth);

    right->merge_point = left->conflicts.end();
    --(right->merge_point);
    left->conflicts.splice(left->conflicts.end(), right->conflicts);
    ++(right->merge_point);
  
    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
        left->num_pos[(*it).first] = left->pos((*it).first) + (*it).second;
    }
    for(num_map::iterator it = right->num_neg.begin();it != right->num_neg.end(); ++it){
        left->num_neg[(*it).first] = left->neg((*it).first) + (*it).second;
    }
    /*for(int i = 0; i < alphabet_size; ++i){
        if(left->child(i) == 0){
            left->child(i) = right->child(i);
            if (right->child(i) != 0){
                right->child(i)->old_depth = right->child(i)->depth;
                right->child(i)->depth = left->depth + 1;
            }
        } else if(right->child(i) != 0){
            apta_node* child = left->child(i)->find();
            apta_node* other_child = right->child(i)->find();
            
            if(child != other_child){
                other_child->det_undo[i] = right;
                merge(child, other_child);
            }
        }
    }*/
    for(child_map::iterator it = right->children.begin();it != right->children.end(); ++it){
        int i = (*it).first;
        apta_node* right_child = (*it).second;
        if(left->child(i) == 0){
            left->children[i] = right_child;
            //right_child->old_depth = right_child->depth;
            //right_child->depth = left->depth + 1;
        } else {
            apta_node* child = left->children[i]->find();
            apta_node* other_child = right_child->find();
            
            if(child != other_child){
                other_child->det_undo[i] = right;
                merge(child, other_child);
            }
        }
    }
}

void state_merger::undo_merge(apta_node* left, apta_node* right){
    /*for(int i = alphabet_size - 1; i >= 0; --i){
        if(left->child(i) == right->child(i)){
            left->children[i] = 0;
            if(right->child(i) != 0){
                right->child(i)->depth = right->child(i)->old_depth;
            }
        }
        else if(left->child(i) != 0 && right->child(i) != 0){
            apta_node* other_child = right->child(i)->find_until(right, i);
            apta_node* child = other_child->representative;
            if(child != other_child)
                undo_merge(child, other_child);
            other_child->det_undo[i] = 0;
        }
    }*/
    for(child_map::reverse_iterator it = right->children.rbegin();it != right->children.rend(); ++it){
        int i = (*it).first;
        apta_node* right_child = (*it).second;
        if(left->child(i) == right_child){
            left->children.erase(i);
            //right_child->depth = right_child->old_depth;
        }
        else if(left->child(i) != 0){
            apta_node* other_child = right_child->find_until(right, i);
            apta_node* child = other_child->representative;
            if(child != other_child)
                undo_merge(child, other_child);
            other_child->det_undo.erase(i);
        }
    }
    
    left->size          -= right->size;
    left->num_accepting -= right->num_accepting;
    left->num_rejecting -= right->num_rejecting;
    left->accepting_paths -= right->accepting_paths;
    left->rejecting_paths -= right->rejecting_paths;
    
    //left->depth = left->old_depth;

    for(num_map::iterator it = right->num_pos.begin();it != right->num_pos.end(); ++it){
        left->num_pos[(*it).first] = left->pos((*it).first) - (*it).second;
    }
    for(num_map::iterator it = right->num_neg.begin();it != right->num_neg.end(); ++it){
        left->num_neg[(*it).first] = left->neg((*it).first) - (*it).second;
    }
    
    right->conflicts.splice(right->conflicts.begin(), left->conflicts, right->merge_point, left->conflicts.end());

    right->representative = 0;
}

/* update structures after performing a merge */
void state_merger::update(){
    state_set new_red;
    state_set new_blue;
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        new_red.insert((*it)->find());
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
bool state_merger::extend_red(){
    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        apta_node* blue = *it;
        bool found = false;
        if(!MERGE_SINKS_DSOLVE && (sink_type(blue) != -1)) continue;
        
        for(state_set::iterator it2 = red_states.begin(); it2 != red_states.end(); ++it2){
            apta_node* red = *it2;

            int score = test_merge(red, blue);
            if(score != -1) found = true;
        }
        
        if(found == false){
            blue_states.erase(blue);
            red_states.insert(blue);
            
            for(int i = 0; i < alphabet_size; ++i){
                apta_node* child = blue->get_child(i);
                if(child != 0) blue_states.insert(child);
            }
            return true;
        }
    }
    return false;
}

bool state_merger::perform_merge(apta_node* left, apta_node* right){
    eval->reset(this);
    merge(left->find(), right->find());
    if(eval->compute_consistency(this, left, right) == false){
        undo_merge(left->find(),right->find());
        return false;
    }
    update();
    return true;
}

int state_merger::test_merge(apta_node* left, apta_node* right){
    eval->reset(this);
    int result = -1;
    if(eval->compute_before_merge) result = eval->compute_score(this, left, right);
    merge(left,right);
    if(!eval->compute_before_merge) result = eval->compute_score(this, left, right);
    if(eval->compute_consistency(this, left, right) == false) result = -1;
    undo_merge(left,right);
    return result;
}

merge_map &state_merger::get_possible_merges(){
    merge_map* mset = new merge_map();
    
    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        apta_node* blue = *(it);
        if(!MERGE_SINKS_DSOLVE && (sink_type(blue) != -1)) continue;
        
        for(state_set::iterator it2 = red_states.begin(); it2 != red_states.end(); ++it2){
            apta_node* red = *it2;
            
            int score = test_merge(red,blue);
            if(score > -1){
                mset->insert(pair<int, pair<apta_node*, apta_node*> >(score, pair<apta_node*, apta_node*>(red, blue)));
            }
        }
    }
    return *mset;
}

void state_merger::todot(FILE* output){
    state_set candidates = get_candidate_states();
    //state_set sinks = get_sink_states();

    fprintf(output,"digraph DFA {\n");
    fprintf(output,"\t%i [label=\"root\" shape=box];\n", aut->root->find()->number);
    fprintf(output,"\t\tI -> %i;\n", aut->root->find()->number);
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        apta_node* n = *it;
        if(n->num_accepting != 0)
            fprintf(output,"\t%i [shape=doublecircle label=\"[%i:%i]\"];\n", n->number, n->num_accepting + n->accepting_paths, n->num_rejecting + n->rejecting_paths);
        else if(n->num_rejecting != 0)
            fprintf(output,"\t%i [shape=Mcircle label=\"[%i:%i]\"];\n", n->number, n->num_accepting + n->accepting_paths, n->num_rejecting + n->rejecting_paths);
        else
            fprintf(output,"\t%i [shape=circle label=\"[%i:%i]\"];\n", n->number, n->num_accepting + n->accepting_paths, n->num_rejecting + n->rejecting_paths);
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
                     childnodes.insert(child);
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
        for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            fprintf(output, "\t\t%i -> %i [label=\"" ,n->number, child->number);
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    fprintf(output, " %s [%i:%i]", aut->alph_str(i).c_str(), n->num_pos[i], n->num_neg[i]);
                }
            }
            fprintf(output, "\"];\n");
        }
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
}
