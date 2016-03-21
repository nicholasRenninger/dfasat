
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

state_set &apta::get_states(){
    state_set* states = new state_set();
    add_states(root->find(), *states);
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
    if(left == 0 || right == 0) return;

    bool consistent = eval->consistent(this, left, right);
    if(consistent) eval->update_score(this, left, right);
    else return;

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
                merge(child, other_child);
            }
        }
    }
}

void state_merger::undo_merge(apta_node* left, apta_node* right){
    if(right->representative != left) return;

    for(child_map::reverse_iterator it = right->children.rbegin();it != right->children.rend(); ++it){
        int i = (*it).first;
        apta_node* right_child = (*it).second;
        if(left->child(i) == right_child){
            left->children.erase(i);
        }
        else if(left->child(i) != 0){
            apta_node* other_child = right_child->find_until(right, i);
            apta_node* child = other_child->representative;
            if(child != other_child)
                undo_merge(child, other_child);
            other_child->det_undo.erase(i);
        }
    }

    left->size -= right->size;
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

            int score = testmerge(red, blue);
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

int state_merger::testmerge(apta_node* left, apta_node* right){
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
    
    apta_node* max_blue = 0;
    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        if(!MERGE_SINKS_DSOLVE && (sink_type(*it) != -1)) continue;
        if(max_blue == 0 || max_blue->occs.size() < (*it)->occs.size())
            max_blue = *it;
    }

    for(state_set::iterator it = blue_states.begin(); it != blue_states.end(); ++it){
        apta_node* blue = *(it);
        if(!MERGE_SINKS_DSOLVE && (sink_type(blue) != -1)) continue;
        
        if(*it != max_blue) continue;

        for(state_set::iterator it2 = red_states.begin(); it2 != red_states.end(); ++it2){
            apta_node* red = *it2;

            int score = testmerge(red,blue);
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
}
