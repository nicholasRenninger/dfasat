
#ifndef _INPUTDATA_H_
#define _INPUTDATA_H_

class inputdata;
class tail;
class tail_wrapper;

#include <istream>
#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <json.hpp>

// for convenience
using json = nlohmann::json;
using namespace std;

// sequence-index pairs
//typedef pair<int,int> tail;
//typedef vector<tail> tail_list;
//typedef set<tail> tail_set;

#include "apta.h"

class tail{
public:
    int sequence;
    int index;
    
    tail* future_tail;
    tail* past_tail;
    tail* next_in_list;
    tail* split_from;
    tail* split_to;
    
    tail(int seq, int i, tail* past_tail);

    tail* split();
    void undo_split();
    tail* next();
    tail* future();
};

class inputdata{
public:

    static json all_data;

    static vector<string> alphabet;
    static map<string, int> r_alphabet;
    
    static int num_attributes;
    int node_number;

    void read_json_file(istream &input_stream);
    void read_abbadingo_file(istream &input_stream);
    void read_abbadingo_sequence(istream &input_stream, int);

    static inline int get_type(int seq_nr){
        return inputdata::all_data[seq_nr]["T"];
    };
    static inline int get_length(int seq_nr){
        return inputdata::all_data[seq_nr]["L"];
    };
    static inline int get_symbol(int seq_nr, int index){
        return inputdata::all_data[seq_nr]["S"][index];
    };
    static inline int get_value(int seq_nr, int index, int attr){
        return inputdata::all_data[seq_nr]["V" + to_string(attr)][index];
    };
    static inline string get_data(int seq_nr, int index){
        if(inputdata::all_data[seq_nr].find("D") != inputdata::all_data[seq_nr].end())
            return inputdata::all_data[seq_nr]["D"][index];
        return "";
    };
    
    static inline int get_type(tail* t){
        return inputdata::all_data[t->sequence]["T"];
    };
    static inline int get_length(tail* t){
        return inputdata::all_data[t->sequence]["L"];
    };
    static inline int get_symbol(tail* t){
        return inputdata::all_data[t->sequence]["S"][t->index];
    };
    static inline int get_value(tail* t, int attr){
        return inputdata::all_data[t->sequence]["V" + to_string(attr)][t->index];
    };
    static inline string get_data(tail* t){
        if(inputdata::all_data[t->sequence].find("D") != inputdata::all_data[t->sequence].end())
            return inputdata::all_data[t->sequence]["D"][t->index];
        return "";
    };
	
    void add_data_to_apta(apta* the_apta);
    void add_sequence_to_apta(apta* the_apta, int seq_nr);

    const string to_json_str() const;
	//const string to_abbadingo_str() const;

    // to init counters etc
    inputdata();
};

#endif /* _INPUTDATA_H_*/
