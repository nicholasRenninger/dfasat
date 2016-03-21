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

using namespace std;

/* constructors and destructors */
apta::apta(ifstream &input_stream){
    int num_words;
    int num_alph = 0;
    map<string, int> seen;
    int node_number = 1;
    input_stream >> num_words >> alphabet_size;
    root = new apta_node();
    max_depth = -1;
    
    for(int line = 0; line < num_words; line++){
        int type;
        int length;
        apta_node* node = root;
        root->depth = 0;
        input_stream >> type >> length;
        
        int depth = 0;
        for(int index = 0; index < length+1; index++){
            depth++;
            string tuple;
            input_stream >> tuple;
            
            std::stringstream lineStream;
            lineStream.str(tuple);
            
            string symbol;
            std::getline(lineStream,symbol,'/');
            string data;
            std::getline(lineStream,data);
            
            node->data.add_data(type, index, length, symbol, data);
            
            if(seen.find(symbol) == seen.end()){
                alphabet[num_alph] = symbol;
                seen[symbol] = num_alph;
                num_alph++;
            }

            int c = seen[symbol];
            if(node->child(c) == 0){
                apta_node* next_node = new apta_node();
                node->children[c] = next_node;
                next_node->source = node;
                next_node->label  = c;
                next_node->number = node_number++;
                next_node->depth = depth;
            }
            node = node->child(c);
        }
        if(depth > max_depth) max_depth = depth;
    }
}

apta::~apta(){
    delete root;
}

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
}

apta_node::~apta_node(){
    for(child_map::iterator it = children.begin();it != children.end(); ++it){
        delete (*it).second;
    }
}
