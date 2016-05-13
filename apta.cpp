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
    root = new apta_node();
    max_depth = 0;
    merge_count = 0;
}

apta::~apta(){
    delete root;
}

/*void apta::read_file(ifstream &input_stream){
    int num_words;
    int num_alph = 0;
    map<string, int> seen;
    int node_number = 1;
    input_stream >> num_words >> alphabet_size;
    
    for(int line = 0; line < num_words; line++){
        int type;
        int length;
        apta_node* node = root;
        root->depth = 0;
        input_stream >> type >> length;
        
        int depth = 0;
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
            
            node->add_target(c);
            node->data->read_from(type, index, length, c, data);
            
            node = node->child(c);
            node->data->read_to(type, index, length, c, data);
        }
        if(depth > max_depth) max_depth = depth;
        node->type = type;
    }
};*/

string apta::alph_str(int i){
    return alphabet[i];
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
    
    try {
       data = (DerivedDataRegister<evaluation_data>::getMap())->at(eval_string)();
    } catch(const std::out_of_range& oor ) {
       std::cerr << "No data type found..." << std::endl;
    }
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

apta_node::~apta_node(){
    for(child_map::iterator it = children.begin();it != children.end(); ++it){
        delete (*it).second;
    }
    delete data;
}


