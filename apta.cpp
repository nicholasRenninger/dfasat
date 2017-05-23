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
    root->red = true;
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
        node->size = node->size + 1;
    }
};

bool is_sink(apta_node* node){
    //cerr << node->size << endl;
    return node->data->sink_type(node) != -1;
}

int apta::sink_type(apta_node* node) {
    return node->data->sink_type(node);
}

void apta::print_dot(iostream& output){
    output << "digraph DFA {\n";
    output << "\t" << root->find()->number << " [label=\"root\" shape=box];\n";
    output << "\t\tI -> " << root->find()->number << ";\n";
    for(merged_APTA_iterator_func Ait = merged_APTA_iterator_func(root, is_sink); *Ait != 0; ++Ait){
    //for(merged_APTA_iterator Ait = merged_APTA_iterator(root); *Ait != 0; ++Ait){
        apta_node* n = *Ait;
        output << "\t" << n->number << " [ label=\"";
        n->data->print_state_label(output, this);
        output << "#" << n->size;
        output << "\" ";
        n->data->print_state_style(output, this);
        if(n->red == false) output << " style=dotted";
        output << " ];\n";

        // transition labels for item
        map<apta_node*, set<int>> childlabels;
        map<int, set<int>> sinklabels;

        for(child_map::iterator it = n->children.begin(); it != n->children.end(); ++it){
            apta_node* child = (*it).second->find();
            if( sink_type(child) != -1){
                if(sinklabels.find(sink_type(child)) == sinklabels.end())
                    sinklabels[sink_type(child)] = set<int>();
                sinklabels[sink_type(child)].insert( it->first );
            } else {
                if(childlabels.find(child) == childlabels.end())
                    childlabels[child] = set<int>();
                childlabels[child].insert( it->first );
            }
        }
        for(map<apta_node*, set<int>>::iterator it2 = childlabels.begin(); it2 != childlabels.end(); ++it2){
            apta_node* child = (*it2).first;
            set<int> labels  = (*it2).second;
            
            output << "\t\t" << n->number << " -> " << child->number << " [label=\"";
            
            for(set<int>::iterator it3 = labels.begin(); it3 != labels.end(); ++it3){
                output << alph_str(*it3) << ":";
                n->data->print_transition_label(output, *it3, this);
                output << "\n";
            }

            output << "\" ";
            n->data->print_transition_style(output, labels, this);
            output << " ];\n";
        }
        continue;
        for(map<int, set<int>>::iterator it2 = sinklabels.begin(); it2 != sinklabels.end(); ++it2){
            int stype = (*it2).first;
            set<int> labels  = (*it2).second;
            
            output << "\tS" << n->number << "t" << stype << " [ label=\"";
            for(set<int>::iterator it3 = labels.begin(); it3 != labels.end(); ++it3){
                output << n->get_child(*it3)->size << " ";
                n->get_child(*it3)->data->print_state_label(output, this);
                output << "\n";
            }
            output << "\" ";
            n->get_child(*(labels.begin()))->data->print_state_style(output, this);
            if(n->red == false) output << " style=dotted";
            output << " ];\n";

            output << "\t\t" << n->number << " -> S" << n->number << "t" << stype << " [ label=\"";
            
            for(set<int>::iterator it3 = labels.begin(); it3 != labels.end(); ++it3){
                output << alph_str(*it3) << ":";
                n->data->print_transition_label(output, *it3, this);
                output << "\n";
            }

            output << "\" ";
            n->data->print_transition_style(output, labels, this);
            output << " ];\n";
        }
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
    size = 0;
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
        source = source->source;
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

void blue_state_iterator::increment() {
    apta_node* next = next_backward();
    while(next != 0){
        while(next != 0){
            current = next;
            if(!current->red) return;
            next = next_forward();
        }
        next = next_backward();
    }
    current = 0;
}

blue_state_iterator::blue_state_iterator(apta_node* start) :
    merged_APTA_iterator(start) {
    apta_node* next = start;
    while(next != 0){
        while(next != 0){
            current = next;
            if(!current->red) return;
            next = next_forward();
        }
        next = next_backward();
    }
    current = 0;
}

void red_state_iterator::increment() {
    apta_node* next = next_forward();
    if(next != 0){
        current = next;
        if(current->red) return;
    }
    next = next_backward();
    while(next != 0){
        current = next;
        if(current->red) return;
        next = next_backward();
    }
    current = 0;
}

red_state_iterator::red_state_iterator(apta_node* start) :
    merged_APTA_iterator(start) {
}

void merged_APTA_iterator_func::increment() {
    apta_node* next = next_forward();
    if(next != 0){
        current = next;
        if(!check_function(current)) return;
    }
    next = next_backward();
    while(next != 0){
        current = next;
        if(!check_function(current)) return;
        next = next_backward();
    }
    current = 0;
}

merged_APTA_iterator_func::merged_APTA_iterator_func(apta_node* start, bool(*node_check)(apta_node*)) : merged_APTA_iterator(start){
    check_function = node_check;
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
