#include "state_merger.h"
#include "evaluate.h"
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <limits.h>

#include "overlap4logs.h"
#include "parameters.h"

REGISTER_DEF_DATATYPE(overlap4logs_data);
REGISTER_DEF_TYPE(overlap4logs);

// they should be parameters to the program
// (they were in Rick's branch)
long DELAY_COLOR_BOUND1 = 5000;
long DELAY_COLOR_BOUND2 = 10000;

const char* colors[] = { "gray", "green", "orange", "cyan", "red" };
const char* colors_labels[] = { "black", "goldenrod", "crimson" };
const char* colors_nodes[] = { "lightgray", "palegreen", "peachpuff", "lightskyblue1", "lightpink" };
const char* names[] = { "?", "A", "D", "C", "E" };

/*** Data operations ***/
overlap4logs_data::overlap4logs_data() {
    num_type = num_map();
    num_delays = num_long_map();
    trace_ids = set<string>();
};

void overlap4logs_data::store_id(string id) {
   trace_ids.insert(id);
};

void overlap4logs_data::print_state_label(iostream& output, apta* aptacontext){
    if(sink_type(node)){
        int sum_types = 0;
        for(num_map::iterator it = num_type.begin(); it != num_type.end(); ++it)
            sum_types += (*it).second;
        output << "[" << sum_types <<"]\n[ ";
        int largest_type = 0;
        for (int i = 0; i < num_sink_types(); i++)
            output << types(i) << " ";
        output << "]";
    } else {
        int stype = sink_type(node);
        output << "sink " << names[stype];
    }
};

void overlap4logs_data::print_state_style(iostream& output, apta* aptacontext){
    if(sink_type(node)){
        int largest_type = 0;
        int count = -1;
        for(num_map::iterator it = num_type.begin(); it != num_type.end(); ++it){
            if((*it).second > count){
                largest_type = (*it).first;
                count = (*it).second;
            }
        }
        output << " style=filled fillcolor=" << colors_nodes[largest_type] << " ";
    } else {
        int stype = sink_type(node);
        output << " shape=box style=filled fillcolor=" << colors[stype] << " tooltip=\"";
        for(int i = 0; i < alphabet_size; ++i){
            if(node->get_child(i) != 0 && sink_type(node->get_child(i)) == stype){
                apta_node* c = node->get_child(i);
                for(set<string>::iterator it3 = reinterpret_cast<overlap4logs_data*>(c->data)->trace_ids.begin(); it3 != reinterpret_cast<overlap4logs_data*>(c->data)->trace_ids.end(); ++it3){
                    output <<  it3->c_str() << " ";
                }
            }
        }
    }
};

void overlap4logs_data::print_transition_label(iostream& output, int symbol, apta* aptacontext){
    if(node->get_child(symbol) != 0){
        int total = pos(symbol) + neg(symbol);
        output << " " << aptacontext->alph_str(symbol).c_str() << " (" << total << "=" << (((double)total*100)/(double)aptacontext->root->find()->size) << "%)\n";

        long minDelay = LONG_MAX;
        long maxDelay = -1;
        double meanDelay = delay_mean(symbol);
        double stdDelay = delay_std(symbol);

        for(long_map::iterator delay_it = num_delays[symbol].begin(); delay_it != num_delays[symbol].end(); delay_it++) {
            if(delay_it->first < minDelay && delay_it->second > 0) {
                minDelay = delay_it->first;
            }
            if(delay_it->first > maxDelay && delay_it->second > 0) {
                maxDelay = delay_it->first;
            }
        }

        output << std::setprecision(0) << "MIN:" << minDelay << " MAX:" << maxDelay;
        output << std::setprecision(1) << " MEAN:" << meanDelay << " STD:" << stdDelay << "\n";
        output << std::setprecision(2);
    }
};

// this should have a pair, set<pair<int, eval_data*>>
void overlap4logs_data::print_transition_style(iostream& output, set<int> symbols, apta* aptacontext){
    int root_size = aptacontext->root->find()->size;
    int edge_sum = 0;
    for(set<int>::iterator it = symbols.begin(); it != symbols.end(); ++it){
        edge_sum += pos(*it) + neg(*it);
    }
    float penwidth = 0.5 + max(0.1, ((double)edge_sum*10)/(double)root_size);

    int color = 0;

    long minDelay = LONG_MAX;
    long maxDelay = -1;

    for(set<int>::iterator sym_it = symbols.begin(); sym_it != symbols.end(); sym_it++){
        int symbol = *sym_it;
        double meanDelay = delay_mean(symbol);
        double stdDelay = delay_std(symbol);
        for(long_map::iterator delay_it = num_delays[symbol].begin(); delay_it != num_delays[symbol].end(); delay_it++) {
            if(delay_it->first < minDelay && delay_it->second > 0) {
                minDelay = delay_it->first;
            }
            if(delay_it->first > maxDelay && delay_it->second > 0) {
                maxDelay = delay_it->first;
            }
        }
        if (meanDelay >= DELAY_COLOR_BOUND2) {
            color = 2;
            break;
        }
        else if (meanDelay >= DELAY_COLOR_BOUND1) {
            color = 1;
        }
    }

    output << " penwidth=\"" << std::setprecision(1) << penwidth << std::setprecision(2) << "\" color=" << colors_labels[color] << " fontcolor=" << colors_labels[color] << " ";
};

void overlap4logs_data::read_from(int type, int index, int length, int symbol, string data){
    overlap_data::read_from(type, index, length, symbol, data);
    if(type >= 1){
        // Store the final outcome
        num_type[type] = types(type) + 1;
    }

    // data is the number of seconds
    long delay = std::stol(data);
    if(num_delays.count(symbol) == 0) {
        num_delays[symbol] = long_map();
    }
    if(num_delays[symbol].count(delay) == 0) {
        num_delays[symbol][delay] = 0;
    }
    num_delays[symbol][delay] += 1;
};

double overlap4logs_data::delay_mean(int symbol){
    long sum = 0, count = 0;

    for(long_map::iterator delay_it = num_delays[symbol].begin(); delay_it != num_delays[symbol].end(); delay_it++) {
        count += delay_it->second;
        sum += delay_it->first * delay_it->second;
    }

    return (double)sum/(double)count;
}

double overlap4logs_data::delay_std(int symbol){
    long count = 0;
    double mean, standardDeviation = 0.0;

    mean = delay_mean(symbol);

    for(long_map::iterator delay_it = num_delays[symbol].begin(); delay_it != num_delays[symbol].end(); delay_it++) {
        count += delay_it->second;
        for(int i = 0; i < delay_it->second; i++) {
            standardDeviation += pow(delay_it->first - mean, 2);
        }
    }

    return sqrt(standardDeviation / count);
};

void overlap4logs_data::update(evaluation_data* right){
    if(node_type == -1) {
       node_type = right->node_type;
       undo_pointer = right;
       trace_ids.insert(reinterpret_cast<overlap4logs_data*>(right)->trace_ids.begin(), reinterpret_cast<overlap4logs_data*>(right)->trace_ids.end());
    } 
    overlap_data::update(right);
    overlap4logs_data* other = (overlap4logs_data*)right;
    for(num_map::iterator it = other->num_type.begin();it != other->num_type.end(); ++it){
        num_type[(*it).first] = types((*it).first) + (*it).second;
    }

    for(num_long_map::iterator it2 = other->num_delays.begin(); it2 != other->num_delays.end(); ++it2){
        int symbol = (*it2).first;
        if(num_delays.count(symbol) == 0) {
            num_delays[symbol] = other->num_delays[symbol];
            continue;
        }

        for(long_map::iterator it3 = other->num_delays[symbol].begin(); it3 != other->num_delays[symbol].end(); ++it3){
            long delay = (*it3).first;

            num_delays[symbol][delay] += (*it3).second;
        }
    }
};

void overlap4logs_data::undo(evaluation_data* right){

    if(right == undo_pointer) {
        for (set<string>::iterator rit = reinterpret_cast<overlap4logs_data*>(right)->trace_ids.begin(); rit != reinterpret_cast<overlap4logs_data*>(right)->trace_ids.end(); ++rit) {
            trace_ids.erase(rit->c_str());
        }
    }
    overlap_data::undo(right);
    overlap4logs_data* other = (overlap4logs_data*)right;
    for(num_map::iterator it = other->num_type.begin();it != other->num_type.end(); ++it){
        num_type[(*it).first] = types((*it).first) - (*it).second;
    }

    // possible memory leak
    for(num_long_map::iterator it2 = other->num_delays.begin(); it2 != other->num_delays.end(); ++it2){
        int symbol = (*it2).first;

        for(long_map::iterator it3 = other->num_delays[symbol].begin(); it3 != other->num_delays[symbol].end(); ++it3){
            long delay = (*it3).first;

            num_delays[symbol][delay] -= (*it3).second;
        }
    }
};

/*** Merge consistency ***/
bool overlap4logs::consistent(state_merger *merger, apta_node* left, apta_node* right){
    if(overlap_driven::consistent(merger, left, right) == false){ inconsistency_found = true; return false; }
    overlap4logs_data* l = (overlap4logs_data*) left->data;
    overlap4logs_data* r = (overlap4logs_data*) right->data;

    //int i = 1;
    for (int i = 0; i < num_sink_types(); i++) {
        if ((l->types(i) > 0 && r->types(i) == 0) || (r->types(i) > 0 && l->types(i) == 0)) {
            inconsistency_found = true;
	    return false;
        }
    }
    
    return true;
};

/*** Sink logic ***/
int overlap4logs_data::find_end_type(apta_node* node) {
    int endtype = -1;

    // Check for all outgoing transitions if there is _one_ unique final type
    for (int z = 0; z < alphabet_size; z++) {
        apta_node* n = node->get_child(z);
        if (n == 0) continue;

        for (int i = 0; i < num_sink_types(); i++) {
            if (((overlap4logs_data*) n->data)->types(i) > 0) {
                if (endtype == -1) {
                    endtype = i;
                }
                // If the outcome is ambiguous, this should not be a sink node
                else if (endtype != i) {
                    return -1;
                }
            }
        }
    }
    return endtype;
}

int overlap4logs_data::sink_type(apta_node* node){
    if(!USE_SINKS) return -1;

    // For a final node, the type itself is the sink type
    if (node->type > 0 && node->type <= num_sink_types()) return node->type;

    // If we want to consider this as a sink node, make sure it's an unambiguous one
    if (node->find()->size > STATE_COUNT) return find_end_type(node);

    return -1;
};

bool overlap4logs_data::sink_consistent(apta_node* node, int type){
    if(!USE_SINKS) return false;

    if(node->type > 0) return node->type == type;
    
    if(type >= 0) return (node->find()->size >STATE_COUNT && find_end_type(node) == type);
    return true;
};

int overlap4logs_data::num_sink_types(){
    if(!USE_SINKS) return 0;
    return 5;
};

/*** Output logic ***/
int overlap4logs::print_labels(iostream& output, apta* aut, overlap4logs_data* data, int symbol) {
    int total = data->pos(symbol) + data->neg(symbol);
    output << " " << aut->alph_str(symbol).c_str() << " (" << total << "=" << (((double)total*100)/(double)aut->root->find()->size) << "%)\n";

    long minDelay = LONG_MAX;
    long maxDelay = -1;
    double meanDelay = data->delay_mean(symbol);
    double stdDelay = data->delay_std(symbol);

    for(long_map::iterator delay_it = data->num_delays[symbol].begin(); delay_it != data->num_delays[symbol].end(); delay_it++) {
        if(delay_it->first < minDelay && delay_it->second > 0) {
            minDelay = delay_it->first;
        }
        if(delay_it->first > maxDelay && delay_it->second > 0) {
            maxDelay = delay_it->first;
        }
    }

    output << std::setprecision(0) << "MIN:" << minDelay << " MAX:" << maxDelay;
    output << std::setprecision(1) << " MEAN:" << meanDelay << " STD:" << stdDelay << "\n";
    output << std::setprecision(2);

    if (meanDelay >= DELAY_COLOR_BOUND2) {
        return 2;
    }
    else if (meanDelay >= DELAY_COLOR_BOUND1) {
        return 1;
    }
    else {
        return 0;
    }
}

/*
void overlap4logs::print_dot(iostream& output, state_merger* merger){
    apta* aut = merger->aut;
    state_set s  = merger->red_states;

    // Some colors and names for the sinks
    const char* colors[] = { "gray", "green", "orange", "cyan", "red" };
    const char* colors_labels[] = { "black", "goldenrod", "crimson" };
    const char* colors_nodes[] = { "lightgray", "palegreen", "peachpuff", "lightskyblue1", "lightpink" };
    const char* names[] = { "?", "A", "D", "C", "E" };

    cerr << "size: " << s.size() << endl;

    output << std::fixed << std::setprecision(2);

    output << "digraph DFA {\n";
    output << "\t" << aut->root->find()->number << " [label=\"root\" shape=box];\n";
    output << "\t\tI -> " << aut->root->find()->number << ";\n";

    int root_size = aut->root->find()->size;

    // Loop through all red states
    for(state_set::iterator it = s.begin(); it != s.end(); ++it){
        apta_node* n = *it;
        output << "\t" << n->number << " [shape=ellipse label=\"[" << n->size << "]\n[ ";
	int largest_type = 0;
	for (int i = 0; i < num_sink_types(); i++) {
		output << ((overlap4logs_data*)n->data)->types(i) << " ";
		if (((overlap4logs_data*)n->data)->types(i) > ((overlap4logs_data*)n->data)->types(largest_type)) {
			largest_type = i;
		}
	}
//	fprintf(output, "]\n[");
//	for (int i = 0; i < num_sink_types(); i++) {
//		fprintf(output, "%.2f%% ", ((double)((overlap4logs_data*)n->data)->types(i)*100)/(double)root_size);
//	}
	output << "]\" style=filled fillcolor=" << colors_nodes[largest_type] << "];\n";
        state_set childnodes;
        set<int> sinks;
        // Determine whether children are sinks or nodes 
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
        // Print sink types and edges to those sinks
        for(set<int>::iterator it2 = sinks.begin(); it2 != sinks.end(); ++it2){
            int stype = *it2;
            output << "\tS" << n->number << "t" << stype << " [label=\"sink " << names[stype] << "\" shape=box style=filled fillcolor=" << colors[stype] << " tooltip=\"";

            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    apta_node* c = n->get_child(i);
                    for(set<string>::iterator it3 = reinterpret_cast<overlap4logs_data*>(c->data)->trace_ids.begin(); it3 != reinterpret_cast<overlap4logs_data*>(c->data)->trace_ids.end(); ++it3){
                        output <<  it3->c_str() << " ";
                    }
                }
            }

            output << "\"];\n";
            output << "\t\t" << n->number << " -> S" << n->number << "t" << stype << " [label=\"";
            int edge_sum = 0;
            int color = 0;
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && sink_type(n->get_child(i)) == stype){
                    overlap4logs_data* node_data = (overlap4logs_data*)n->data;
                    edge_sum += node_data->pos(i) + node_data->neg(i);
                    int label_color = print_labels(output, aut, node_data, i);
                    if (label_color > color) {
                        color = label_color;
                    }
                }
            }
            float penwidth = 0.5 + max(0.1, ((double)edge_sum*10)/(double)root_size);
            output << "\" penwidth=" << std::setprecision(1) << penwidth << std::setprecision(2) << " color=" << colors_labels[color] << " fontcolor=" << colors_labels[color] << "];\n";
        }
        // Print edges
        for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            output << "\t\t" << n->number << " -> " << child->number << " [label=\"";
            int edge_sum = 0;
            int color = 0;
            for(int i = 0; i < alphabet_size; ++i){
                if(n->get_child(i) != 0 && n->get_child(i) == child){
                    overlap4logs_data* node_data = (overlap4logs_data*)n->data;
                    edge_sum += node_data->pos(i) + node_data->neg(i);
                    int label_color = print_labels(output, aut, node_data, i);
                    if (label_color > color) {
                        color = label_color;
                    }
                }
            }
            float penwidth = 0.5 + max(0.1, ((double)edge_sum*10)/(double)root_size);
            output << "\" penwidth=" << std::setprecision(1) << penwidth << std::setprecision(2) << " color=" << colors_labels[color] << " fontcolor=" << colors_labels[color] << "];\n";
        }
    }

    output << "}\n";
};
*/

