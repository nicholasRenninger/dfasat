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

apta_node::~apta_node(){
    for(child_map::iterator it = children.begin();it != children.end(); ++it){
        delete (*it).second;
    }
    delete data;
}
