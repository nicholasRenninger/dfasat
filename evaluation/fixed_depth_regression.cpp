#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "fixed_depth_regression.h"
#include "parameters.h"

REGISTER_DEF_DATATYPE(fixed_depth_mse_data);
REGISTER_DEF_TYPE(fixed_depth_mse_error);

int sequence_index = 0;

void fixed_depth_mse_data::read_from(int type, int index, int length, int symbol, string data){
    if(index == 0)
        sequence_index++;
    
    indexes.push_back(sequence_index);
    
    double occ = std::stod(data);
    if(occ > 0.0) mse_data::read_from(type, index, length, symbol, data);
    ald.read_from(type, index, length, symbol, data);
};

void fixed_depth_mse_data::read_to(int type, int index, int length, int symbol, string data){
    double occ = std::stod(data);
    if(occ > 0.0) mse_data::read_to(type, index, length, symbol, data);
    ald.read_to(type, index, length, symbol, data);
};

void fixed_depth_mse_data::update(evaluation_data* right){
    fixed_depth_mse_data* r = (fixed_depth_mse_data*)right;

    mse_data::update(right);
    ald.update(&(r->ald));

    if(indexes.size() != 0){
        r->int_merge_point = indexes.end();
        --(r->int_merge_point);
        indexes.splice(indexes.end(), r->indexes);
        ++(r->int_merge_point);
    } else {
        indexes.splice(indexes.begin(), r->indexes);
        r->int_merge_point = indexes.begin();
    }
};

void fixed_depth_mse_data::undo(evaluation_data* right){
    fixed_depth_mse_data* r = (fixed_depth_mse_data*)right;

    mse_data::undo(right);
    ald.undo(&(r->ald));

    r->indexes.splice(r->indexes.begin(), indexes, r->int_merge_point, indexes.end());
};

void fixed_depth_mse_error::update_score(state_merger *merger, apta_node* left, apta_node* right){
};

bool fixed_depth_mse_error::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(left->depth != right->depth){ inconsistency_found = true; return false; }
    if(left->type != right->type){ inconsistency_found = true; return false; }
    return evaluation_function::consistent(merger,left,right);
    return true;
    
    fixed_depth_mse_data* l = (fixed_depth_mse_data*)left;
    fixed_depth_mse_data* r = (fixed_depth_mse_data*)right;
    
    //if(l->occs.size() == 0 && r->occs.size() != 0){ inconsistency_found = true; return false; }
    //if(l->occs.size() != 0 && r->occs.size() == 0){ inconsistency_found = true; return false; }

    //if(l->num_accepting != 0 && r->num_rejecting != 0){ inconsistency_found = true; return false; }
    //if(l->num_rejecting != 0 && r->num_accepting != 0){ inconsistency_found = true; return false; }
    return mse_error::consistent(merger,left,right);
};

void fixed_depth_mse_error::reset(state_merger *merger){
    l_dist = vector< state_set >(merger->aut->max_depth);
    r_dist = vector< state_set >(merger->aut->max_depth);
    for(int i = 0; i < merger->aut->max_depth; ++i){
        l_dist[i] = state_set();
        r_dist[i] = state_set();
    }
    compute_before_merge = true;
    ale.reset(merger);
    mse_error::reset(merger);
};

void fixed_depth_mse_error::score_right(apta_node* right, int depth){
    if(r_dist[depth].find(right) != r_dist[depth].end()) return;
    //fixed_depth_mse_data* l = (fixed_depth_mse_data*)right->data;
    //cerr << "adding " << l->occs.size() << " to right." << endl;
    r_dist[depth].insert(right);
    if(depth < r_dist.size()){
        for(guard_map::iterator it = right->guards.begin(); it != right->guards.end(); ++it){
            score_right((*it).second->target->find(), depth + 1);
        }
    }
};

/*void fixed_depth_mse_error::undo_score_right(apta_node* right, int depth){
    if(depth < l_dist.size() - 1){
        for(child_map::reverse_iterator it = right->children.rbegin(); it != right->children.rend(); ++it){
            undo_score_right((*it).second->find(), depth + 1);
        }
    }
    r_dist[depth].undo(right->data);
    //fixed_depth_mse_data* l = (fixed_depth_mse_data*)right->data;
    //cerr << "undo adding " << l->occs.size() << " to right." << endl;
};*/

void fixed_depth_mse_error::score_left(apta_node* left, int depth){
    if(l_dist[depth].find(left) != l_dist[depth].end()) return;
    //fixed_depth_mse_data* l = (fixed_depth_mse_data*)left->data;
    //cerr << "adding " << l->occs.size() << " to left." << endl;
    l_dist[depth].insert(left);
    if(depth < l_dist.size()){
        for(guard_map::iterator it = left->guards.begin(); it != left->guards.end(); ++it){
            score_left((*it).second->target->find(), depth + 1);
        }
    }
};

/*void fixed_depth_mse_error::undo_score_left(apta_node* left, int depth){
    if(depth < l_dist.size() - 1){
        for(child_map::reverse_iterator it = left->children.rbegin(); it != left->children.rend(); ++it){
            undo_score_left((*it).second->find(), depth + 1);
        }
    }
    l_dist[depth].undo(left->data);
    //fixed_depth_mse_data* l = (fixed_depth_mse_data*)left->data;
    //cerr << "undo adding " << l->occs.size() << " to left." << endl;
};*/
bool is_all_same_sink(apta_node* node, int type){
    node = node->find();
    for(guard_map::iterator it = node->guards.begin();it != node->guards.end(); ++it){
        int i = (*it).first;
        apta_node* child = (*it).second->target;
        if(type == -1){
            if (child != 0) type = i;
        } else {
            if (i == type) continue;
            if (child != 0) return false;
        }
    }
    for(guard_map::iterator it = node->guards.begin();it != node->guards.end(); ++it){
        int i = (*it).first;
        apta_node* child = (*it).second->target;
        if (i != type) continue;
        if (child != 0 && is_all_same_sink(child, type) == false) return false;
    }
    return true;
};

int fixed_depth_mse_error::compute_score(state_merger *merger, apta_node* left, apta_node* right){
    if(left->depth != right->depth){ return -1; }
    if(is_all_same_sink(left, -1) != is_all_same_sink(right, -1)){ return -1; }
    
    score_right(right,0);

    score_left(left,0);
    
    int tests_passed = 0;
    int tests_failed = 0;

    for(int i = 0; i < merger->aut->max_depth; ++i){
        for(int a = 0; a < alphabet_size; ++a){
            double count_left = 0;
            double count_right = 0;
            double total_left = 0;
            double total_right = 0;
            for(state_set::iterator it = l_dist[i].begin(); it != l_dist[i].end(); ++it){
                apta_node* node = *it;
                fixed_depth_mse_data* l = (fixed_depth_mse_data*)node->data;
                
                count_left += l->ald.pos(a) + l->ald.neg(a);
                total_left += l->ald.accepting_paths + l->ald.rejecting_paths;
            }
            for(state_set::iterator it = r_dist[i].begin(); it != r_dist[i].end(); ++it){
                apta_node* node = *it;
                fixed_depth_mse_data* r = (fixed_depth_mse_data*)node->data;
                
                count_right += r->ald.pos(a) + r->ald.neg(a);
                total_right += r->ald.accepting_paths + r->ald.rejecting_paths;
            }
            //cerr << count_left << "," << total_left << " " << count_right << "," << total_right << endl;

            if(total_left < STATE_COUNT || total_right < STATE_COUNT) continue;
            
            bool alergia_consistent = alergia::alergia_consistency(count_right, count_left, total_right, total_left);
            if(alergia_consistent) tests_passed += 1;
            else tests_failed += 1;
        }
    }
    
    if (tests_failed != 0) return -1;
    int depth_score = 100 - left->depth;
    //return 100*depth_score + tests_passed;
    int num_runs = 0;
    float total_error = 0.0;
    
    
    for(int i = 0; i < merger->aut->max_depth; ++i){
        double mean_left = 0.0;
        double mean_right = 0.0;
        double left_size = 0.0;
        double right_size = 0.0;
        
        for(state_set::iterator it = l_dist[i].begin(); it != l_dist[i].end(); ++it){
            apta_node* node = *it;
            fixed_depth_mse_data* l = (fixed_depth_mse_data*)node->data;
            for(double_list::iterator it2 = l->occs.begin(); it2 != l->occs.end(); ++it2){
                mean_left = mean_left + (double)*it2;
            }
            left_size = left_size + l->occs.size();
        }
    
        for(state_set::iterator it = r_dist[i].begin(); it != r_dist[i].end(); ++it){
            apta_node* node = *it;
            fixed_depth_mse_data* r = (fixed_depth_mse_data*)node->data;
            for(double_list::iterator it2 = r->occs.begin(); it2 != r->occs.end(); ++it2){
                mean_right = mean_right + (double)*it2;
            }
            right_size = right_size + r->occs.size();
        }
        
        if(right_size == 0 || left_size == 0) continue;
        
        double error_left = 0.0;
        double error_right = 0.0;
        double error_total = 0.0;
        double mean_total = 0.0;
    
        mean_total = ((mean_left * left_size) + (mean_right * right_size)) / (left_size + right_size);
    
        for(state_set::iterator it = l_dist[i].begin(); it != l_dist[i].end(); ++it){
            apta_node* node = *it;
            fixed_depth_mse_data* l = (fixed_depth_mse_data*)node->data;
            for(double_list::iterator it = l->occs.begin(); it != l->occs.end(); ++it){
                error_left  = error_left  + ((mean_left  - (double)*it)*(mean_left  - (double)*it));
                error_total = error_total + ((mean_total - (double)*it)*(mean_total - (double)*it));
            }
        }
        for(state_set::iterator it = r_dist[i].begin(); it != r_dist[i].end(); ++it){
            apta_node* node = *it;
            fixed_depth_mse_data* r = (fixed_depth_mse_data*)node->data;
            for(double_list::iterator it = r->occs.begin(); it != r->occs.end(); ++it){
                error_right = error_right + ((mean_right - (double)*it)*(mean_right - (double)*it));
                error_total = error_total + ((mean_total - (double)*it)*(mean_total - (double)*it));
            }
        }
    
        RSS_before += error_right+error_left;
        RSS_after  += error_total;
        num_points += left_size;
        num_points += right_size;
        
        error_total = mean_left - mean_right;
        if(error_total < 0) error_total = -error_total;
        total_error += error_total;
        num_runs = num_runs + 1;
    }
    
    return 10000*depth_score - (total_error/((float)num_runs));
    
    //if(2*total_merges + num_points*(log(RSS_before/num_points)) - num_points*log(RSS_after/num_points) < 0) return -1;
    
    //if(left->source == right->source)
        //return 10000*depth_score + 5*(2*total_merges + num_points*(log(RSS_before/num_points)) - num_points*log(RSS_after/num_points));
    return 10000*depth_score + (2*total_merges + num_points*(log(RSS_before/num_points)) - num_points*log(RSS_after/num_points));
};

int fixed_depth_mse_error::sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    if (is_all_same_sink(node, -1)) return 0;
    return -1;
};

bool fixed_depth_mse_error::sink_consistent(apta_node* node, int type){
    if(!USE_SINKS) return false;
    if(type == 0) return is_all_same_sink(node, -1);
    
    return true;
};

int fixed_depth_mse_error::num_sink_types(){
    if(!USE_SINKS) return 0;
    return 1;
};

/*void fixed_depth_mse_error::print_dot(iostream& output, state_merger* merger){
    apta* aut = merger->aut;
    state_set candidates  = merger->get_candidate_states();
    
    //state_set sinks = get_sink_states();

    output << "digraph DFA {\n";
    
//    fprintf(output,"subgraph cluster_1 {\nstyle=filled;color=green4;");
    state_set* state = &merger->aut->get_merged_states();
    state_set states = *state;
    for(state_set::iterator it = states.begin(); it != states.end(); ++it){
        apta_node* node = *it;
        fixed_depth_mse_data* l = (fixed_depth_mse_data*) node->data;
        if (sink_type(node) == -1 && node->depth != aut->max_depth && l->ald.accepting_paths > 2.0 * l->ald.rejecting_paths){// || l->num_accepting != 0){
            output << node->number << ";";
        }
    }
    //for(int i = 0; i < aut->max_depth; ++i) fprintf(output,"rg%i; ",i);
    delete state;
//    fprintf(output,"\nlabel = \"good\";\n");
//    fprintf(output,"}\n");
//    fprintf(output,"subgraph cluster_2 {\nstyle=filled;color=purple4;");
    state = &merger->aut->get_merged_states();
    states = *state;
    for(state_set::iterator it = states.begin(); it != states.end(); ++it){
        apta_node* node = *it;
        fixed_depth_mse_data* l = (fixed_depth_mse_data*) node->data;
        if (sink_type(node) == -1 &&node->depth != aut->max_depth &&  l->ald.accepting_paths <= 2.0 * l->ald.rejecting_paths
        && l->ald.rejecting_paths <= 2.0 * l->ald.accepting_paths){// || l->num_accepting != 0){
            output << node->number << ";";
        }
    }
    //for(int i = 0; i < aut->max_depth; ++i) fprintf(output,"rgb%i; ",i);
    delete state;
//    fprintf(output,"\nlabel = \"good/bad\";\n");
//    fprintf(output,"}\n");
//    fprintf(output,"subgraph cluster_3 {\nstyle=filled;color=red4;");
    state = &merger->aut->get_merged_states();
    states = *state;
    for(state_set::iterator it = states.begin(); it != states.end(); ++it){
        apta_node* node = *it;
        fixed_depth_mse_data* l = (fixed_depth_mse_data*) node->data;
        if (sink_type(node) == -1 && node->depth != aut->max_depth && l->ald.rejecting_paths > 2.0 * l->ald.accepting_paths){// || l->num_rejecting != 0){
            output << node->number << ";";
        }
    }
    //for(int i = 0; i < aut->max_depth; ++i) fprintf(output,"rb%i; ",i);
    delete state;
//    fprintf(output,"\nlabel = \"bad\";\n");
//    fprintf(output,"}\n");
    state = &merger->aut->get_merged_states();
    states = *state;
    
    for(int i = 0; i < aut->max_depth; ++i) fprintf(output,"rg%i -> rgb%i [style=invis]; ",i,i);
    for(int i = 0; i < aut->max_depth; ++i) fprintf(output,"rgb%i -> rb%i [style=invis]; ",i,i);

    for(state_set::iterator it = states.begin(); it != states.end(); ++it){
        apta_node* node = *it;
        if (sink_type(node) != -1) continue;
        fprintf(output,"rg%i -> %i [style=invis];\n",node->depth, node->number);
        fprintf(output,"rgb%i -> %i [style=invis];\n",node->depth, node->number);
        fprintf(output,"rb%i -> %i [style=invis];\n",node->depth, node->number);
    }
    delete state;
    

    for(int i = 0; i <= aut->max_depth; ++i){
        fprintf(output,"rg%i [style=invis];\n ",i);
        fprintf(output,"rgb%i [style=invis];\n ",i);
        fprintf(output,"rb%i [style=invis];\n ",i);
    }
    for(int i = 0; i <= aut->max_depth-1; ++i){
        fprintf(output,"rg%i->rg%i [style=invis];\n ",i, i+1);
        fprintf(output,"rgb%i->rgb%i [style=invis];\n ",i, i+1);
        fprintf(output,"rb%i->rb%i [style=invis];\n ",i, i+1);
    }


    for(state_set::iterator it = merger->red_states.begin(); it != merger->red_states.end(); ++it){
        apta_node* n = *it;
        fixed_depth_mse_data* l = reinterpret_cast<fixed_depth_mse_data*>(n->data);

        int node_size = 1 + (int)((float)(l->occs.size())/10.0);

        if(l->ald.num_accepting != 0)
            fprintf(output,"\t%i [shape=box style=\"filled\" color=\"green\" label=\"%i\"];\n", n->number, l->ald.num_accepting);
        else if(l->ald.num_rejecting != 0)
            fprintf(output,"\t%i [shape=box style=\"filled\" color=\"red\" label=\"%i\"];\n", n->number, l->ald.num_rejecting);
        else {
            output << "\t" << n->number << " [shape=box nodesep=1 style=\"filled\"";
            if(l->occs.size() == 0)
                output << "color=\"white\"";
            else if(l->mean < 5)
                output << "color=\"red\"";
            else if(l->mean < 5.5)
                output << "color=\"orange\"";
            else if(l->mean < 6)
                output << "color=\"yellow\"";
            else if(l->mean < 7)
                output << "color=\"darkgreen\"";
            else//if(l->mean <= 10.0)
                output << "color=\"green\"";
            output << "label=\"(";
            for(int_list::iterator it2 = l->indexes.begin(); it2 != l->indexes.end(); it2++){
                output << *it2 << ",";
            }
            output << ")\n" << (float)((int)(100*l->mean))/100.0 << "\n" << (int)l->occs.size() << "\n" << l->ald.accepting_paths << ":" << l->ald.rejecting_paths << "\"];\n";
            //else
            //    fprintf(output,"\t%i [shape=ellipse width=%i penlabel=\"\"];\n", n->number, node_size);
        //}
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
            int node_size = int((float)(l->ald.pos(i) + l->ald.neg(i))/3.0) + 2;
            if (aut->alph_str(i) == "fail")
                output << "\t\t" << n->number << " -> " << child->number << " [minlen=1 penwidth=" << node_size << " color=\"red\" label=\"" << l->ald.pos(i) << ":" << l->ald.neg(i) << "\"];\n";
            else if (aut->alph_str(i) == "pass")
                output << "\t\t" << n->number << " -> " << child->number << " [minlen=1 penwidth=" << node_size << " color=\"green\" label=\"" << l->ald.pos(i) << ":" << l->ald.neg(i) << "\"];\n";
            else if (aut->alph_str(i) == "none")
                output << "\t\t" << n->number << " -> " << child->number << " [minlen=1 penwidth=" << node_size << " color=\"grey\" label=\"" << l->ald.pos(i) << ":" << l->ald.neg(i) << "\"];\n";
            else if (aut->alph_str(i) == "already")
                output << "\t\t" << n->number << " -> " << child->number << " [minlen=1 penwidth=" << node_size << " color=\"blue\" label=\"" << l->ald.pos(i) << ":" << l->ald.neg(i) << "\"];\n";
            else
                output << "\t\t" << n->number << " -> " << child->number << " [minlen=1 penwidth=" << node_size << " color=\"black\" label=\"" << l->ald.pos(i) << ":" << l->ald.neg(i) << "\"];\n";
                     //childnodes.insert(child);
                 }
            }
        }
        for(set<int>::iterator it2 = sinks.begin(); it2 != sinks.end(); ++it2){
            int stype = *it2;
            output << "\tS" << n->number << "t" << stype << " [label=\"sink " << stype << "\" shape=box];\n";
            output << "\t\t" << n->number << " -> S" << n->number << "t" << stype << " [label=\"";
            for(int i = 0; i < aut->alphabet.size(); ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    output << " " << aut->alph_str(i).c_str() << " [" << l->ald.num_pos[i] << ":" << l->ald.num_neg[i] << "]";
                }
            }
            output << "\"];\n";
        }

    }
    output << "}\n";
};*/
