#include "apta.h"
#include "state_merger.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>

#include "evaluators.h"
#include "parameters.h"
#include "evaluation_factory.h"

using namespace std;

/* constructors and destructors */
apta::apta(){
    root = new apta_node(this);
    max_depth = 0;
    merge_count = 0;
}

apta::~apta(){
    delete root;
}

/* for batch mode */
// i want tis to map back to the sample by sample read functions from streaming
void apta::read_file(istream &input_stream){
    int num_words;
    int num_alph = 0;
    map<string, int> seen;
    int node_number = 1;
    input_stream >> num_words >> alphabet_size;

    for(int line = 0; line < num_words; line++){
        int type;
        int length;
        input_stream >> type >> length;

        int depth = 0;
        apta_node* node = root;
        for(int index = 0; index < length; index++){
            depth++;
            string tuple;
            input_stream >> tuple;

            std::stringstream lineStream;
            lineStream.str(tuple);

            string symbol;
            std::getline(lineStream,symbol,'/');
            string data;
            std::getline(lineStream,data);

            if(seen.find(symbol) == seen.end()){
                alphabet[num_alph] = symbol;
                seen[symbol] = num_alph;
                num_alph++;
            }
            int c = seen[symbol];
            if(node->child(c) == 0){
                apta_node* next_node = new apta_node(this);
                node->children[c] = next_node;
                next_node->source = node;
                next_node->label  = c;
                next_node->number = node_number++;
                next_node->depth = depth;
            }
            node->size = node->size + 1;
            node->data->read_from(type, index, length, c, data);
            node = node->child(c);
            node->data->read_to(type, index, length, c, data);
        }
        if(depth > max_depth) max_depth = depth;
        node->type = type;
    }
};

void apta::print_dot(iostream& output){
    output << "digraph DFA {\n";
    output << "\t" << root->find()->number << " [label=\"root\" shape=box];\n";
    output << "\t\tI -> " << root->find()->number << ";\n";
    for(merged_APTA_iterator Ait = merged_APTA_iterator(root); *Ait != 0; ++Ait){
        apta_node* n = *Ait;
        output << "\t" << n->number << " [ label=\"";
        n->data->print_state_label(output);
        output << n->size;
        output << "\" ";
        n->data->print_state_style(output);
        if(n->red == false) output << " style=dotted";
        output << " ];\n";

        // items to reach
        state_set childnodes;
        set<int> sinks;
        // transition labels for item
        map<apta_node*, set<int>> labels;
        map<apta_node*, set<int>> sinklabels;

        for(child_map::iterator it = n->children.begin(); it != n->children.end(); ++it){
            apta_node* child = (*it).second;
            if(child->data->sink_type() != -1){
                sinks.insert(child->data->sink_type());
                if(sinklabels.find(child) == sinklabels.end()) {

                } else {
                    sinklabels[child].insert( it->first );
                }

            } else {
                childnodes.insert(child);
                if(labels.find(child) == labels.end()) {

                    labels[child].insert( it->first );
                } else {

                    labels[child].insert( it->first );
                }
            }
        }
        for(state_set::iterator it2 = childnodes.begin(); it2 != childnodes.end(); ++it2){
            apta_node* child = *it2;
            output << "\t\t" << n->number << " -> " << child->number << " [label=\"";

            n->data->print_transition_label(output, n, labels[child], child);

            output << "\" ";
            n->data->print_transition_style(output, child);
            output << " ];\n";
        }

        /*for(set<int>::iterator it = sinks.begin(); it != sinks.end(); ++it){
            int stype = *it;
            output << "\tS" << n->number << "t" << stype << " [ label=\"";
            n->data->print_sink_label(output, stype);
            output << "\" ";
            n->data->print_sink_style(output, stype);
            output << " ];\n";

            output << "\t\t" << n->number << " -> S" << n->number << "t" << stype << " [ label=\"";

            n->data->print_sink_transition_label(output, stype, sinklabels[n], n);
            output << "\" ";
            n->data->print_sink_transition_style(output, stype);
            output << " ];\n";
        }*/
    }
    output << "}\n";
};

string apta::alph_str(int i){
    return alphabet[i];
}

apta_node::apta_node(apta *context) {
    this->context = context;
    source = 0;
    representative = 0;

    children = child_map();
    det_undo = child_map();

    label = 0;
    number = 0;
    satnumber = 0;
    colour = 0;
    size = 1;
    depth = 0;
    type = -1;

    age = 0;

    red = false;

    try {
       data = (DerivedDataRegister<evaluation_data>::getMap())->at(eval_string)();
    } catch(const std::out_of_range& oor ) {
       std::cerr << "No data type found..." << std::endl;
    }

}

apta_node::apta_node(){
    source = 0;
    representative = 0;

    children = child_map();
    det_undo = child_map();

    label = 0;
    number = 0;
    satnumber = 0;
    colour = 0;
    size = 1;
    depth = 0;
    type = -1;

    red = false;

    try {
       data = (DerivedDataRegister<evaluation_data>::getMap())->at(eval_string)();
    } catch(const std::out_of_range& oor ) {
       std::cerr << "No data type found..." << std::endl;
    }
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

/* iterators for the APTA and merged APTA */
APTA_iterator::APTA_iterator(apta_node* start){
    base = start;
    current = start;
}

apta_node* APTA_iterator::next_forward() {
    child_map::iterator it;
    for(it = current->children.begin();it != current->children.end(); ++it){
        if((*it).second->source == current){
            return (*it).second;
        }
    }
    return 0;
}

apta_node* APTA_iterator::next_backward() {
    child_map::iterator it;
    apta_node* source = current;
    while(source != base){
        current = source;
        source = source->source->find();
        it = source->children.find(current->label);
        ++it;
        for(; it != source->children.end(); ++it){
            if((*it).second->source == current){
                return (*it).second;
            }
        }
    }
    return 0;
}

void APTA_iterator::increment() {
    apta_node* next = next_forward();
    if(next != 0){ current = next; return; }
    next = next_backward();
    if(next != 0){ current = next; return; }
    current = 0;
}

merged_APTA_iterator::merged_APTA_iterator(apta_node* start){
    base = start;
    current = start;
}

apta_node* merged_APTA_iterator::next_forward() {
    child_map::iterator it;
    for(it = current->children.begin();it != current->children.end(); ++it){
        if((*it).second->representative == 0){
            return (*it).second;
        }
    }
    return 0;
}

apta_node* merged_APTA_iterator::next_backward() {
    child_map::iterator it;
    apta_node* source = current;
    while(source != base){
        current = source;
        source = source->source->find();
        it = source->children.find(current->label);
        ++it;
        for(; it != source->children.end(); ++it){
            if((*it).second->representative == 0){
                return (*it).second;
            }
        }
    }
    return 0;
}

void merged_APTA_iterator::increment() {
    apta_node* next = next_forward();
    if(next != 0){ current = next; return; }
    next = next_backward();
    if(next != 0){ current = next; return; }
    current = 0;
}

/*apta_node::add_target(int symbol){
    if(node->child(symbol) == 0){
        apta_node* next_node = new apta_node();
        node->children[symbol] = next_node;
        next_node->source = this;
        next_node->label  = symbol;
        next_node->number = apta::node_number++;
        next_node->depth = depth+1;
    }
    size = size + 1;
}*/

std::set<void*> freed;

apta_node::~apta_node(){
    for(child_map::iterator it = children.begin();it != children.end(); ++it){
        if (freed.find((*it).second) != freed.end()) {
            freed.insert((*it).second);
            delete (*it).second;
        }
    }
    delete data;
}
