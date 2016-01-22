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
        apta_node* node = root;
        root->depth = 1;
        int positive;
        int length;
        input_stream >> positive >> length;

        int depth = 0;
        for(int index = 0; index < length; index++){
            depth = depth + 1;
            vector<int> event;
            string tuple;
            input_stream >> tuple;
            std::stringstream lineStream;
            lineStream.str(tuple);
            string cell;
            string event_string;
            if(std::getline(lineStream,cell,','))
            {
                event_string = cell;
                event.push_back(stoi(cell));
            }
            double occ;
            if(std::getline(lineStream,cell,','))
            {
                occ = std::stod(cell);
            }
            tuple = event_string;
            if(seen.find(tuple) == seen.end()){
                alphabet[num_alph] = event;
                seen[tuple] = num_alph;
                num_alph++;
            }
            int c = seen[tuple];
            //cerr << tuple << " " << occ << endl;
            if(node->child(c) == 0){
                apta_node* next_node = new apta_node();
                node->children[c] = next_node;
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
            node->occs.push_front(occ);
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
