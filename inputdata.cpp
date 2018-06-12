/*
 */

#include "inputdata.h"

json inputdata::all_data;
vector<string> inputdata::alphabet;
map<string, int> inputdata::r_alphabet;
int inputdata::num_attributes;

tail* tail::split(){
    tail* t = new tail(sequence, index, 0);
    t->split_from = this;
    t->future_tail = future_tail;
    t->past_tail = past_tail;
    split_to = t;
    return t;
};

void tail::undo_split(){
    split_to = 0;
};

tail* tail::next(){
    if(split_to == 0) return next_in_list;
    if(next_in_list == 0) return 0;
    return next_in_list->next();
};

tail* tail::future(){
    if(split_to == 0) return future_tail;
    return split_to->future();
};

tail::tail(int seq, int i, tail* pt){
    sequence = seq;
    index = i;
    past_tail = pt;
    if(past_tail != 0) past_tail->future_tail = this;
    
    future_tail = 0;
    next_in_list = 0;
    split_from = 0;
};

inputdata::inputdata() {
    node_number = 0;
}

void inputdata::read_json_file(istream &input_stream){
    input_stream >> all_data;
};

void inputdata::read_abbadingo_file(istream &input_stream){
    int num_sequences, num_attributes, alph_size;
    input_stream >> num_sequences;
    
    string tuple;
    input_stream >> tuple;

    std::stringstream lineStream;
    lineStream.str(tuple);

    string alph;
    std::getline(lineStream,alph,':');
    string attr;
    std::getline(lineStream,attr);

    alph_size = stoi(alph);
    if(!attr.empty())
        inputdata::num_attributes = stoi(attr);
    else
        inputdata::num_attributes = 0;
    
    cerr << "ATTR: " << attr << " " << inputdata::num_attributes << endl;
	
	for(int line = 0; line < num_sequences; ++line){
		read_abbadingo_sequence(input_stream, num_attributes);
	}
};

void inputdata::read_abbadingo_sequence(istream &input_stream, int num_attributes){
    int type, length;
    
    json sequence;

    input_stream >> type;
    input_stream >> length;

    sequence["T"] = type;
    sequence["L"] = length;
    
    vector<int> symbols(length);
    vector< vector<int> > values(num_attributes, vector<int>(length));
    vector< string > datas(length);
    
    bool has_data = false;
    
    for(int index = 0; index < length; ++index){
        string tuple;
        input_stream >> tuple;

        std::stringstream l1;
        l1.str(tuple);

        string temp_symbol;
        std::getline(l1,temp_symbol,'/');
        string data;
        std::getline(l1,data);
        if(!data.empty())
            has_data = true;

        std::stringstream l2;
        l2.str(temp_symbol);

        string symbol;
        std::getline(l2,symbol,':');
        string vals;
        std::getline(l2,vals);
        
        //cerr << symbol << " " << vals << endl;
        
        if(r_alphabet.find(symbol) == r_alphabet.end()){
            r_alphabet[symbol] = alphabet.size();
            alphabet.push_back(symbol);
        }
        
        symbols[index] = r_alphabet[symbol];
        datas[index] = data;

        string val;
        if(num_attributes != 0){
            std::stringstream l3;
            l3.str(vals);
            for(int i = 0; i < num_attributes-1; ++i){
                std::getline(l3,val,',');
cerr << val;
                values[i][index] = stof(val);
            }
            std::getline(l3,val);
            values[num_attributes-1][index] = stof(val);
        }
    }
    sequence["S"] = symbols;
    for(int i = 0; i < num_attributes; ++i){
        sequence["V" + to_string(i)] = values[i];
    }
    if(has_data)
        sequence["D"] = datas;
    
    all_data.push_back(sequence);
};

void inputdata::add_data_to_apta(apta* the_apta){
    for(int i = 0; i < all_data.size(); ++i){
        add_sequence_to_apta(the_apta, i);
    }
};

void inputdata::add_sequence_to_apta(apta* the_apta, int seq_nr){
    json sequence = all_data[seq_nr];
    
    int depth = 0;
    apta_node* node = the_apta->root;
    tail* ot = 0;
    
    for(int index = 0; index < sequence["L"]; index++){
        depth++;
        tail* nt = new tail(seq_nr, index, ot);
        int symbol = sequence["S"][index];
        if(node->child(symbol) == 0){
            apta_node* next_node = new apta_node(the_apta);
            node->set_child(symbol, next_node);
            next_node->source = node;
            next_node->label  = symbol;
            next_node->depth  = depth;
            next_node->number = ++(this->node_number);
        }
        node->size = node->size + 1;
        node->add_tail(nt);
        node->data->read_from(seq_nr, index);
        node = node->child(symbol);
        node->data->read_to(seq_nr, index);
        ot = nt;
    }
    node->type = sequence["T"];
    node->size = node->size + 1;
};

const string inputdata::to_json_str() const{
    ostringstream ostr;
    ostr << all_data;
    return ostr.str();
};

/*
const string inputdata::to_abbadingo_str() const{
    ostringstream ostr;
	ostr << num_sequences  << " " << alph_size << ":" << num_attributes << "\n";
	for(int line = 0; line < num_sequences; ++line) {
	    sequence* seq = sequences[line];
	    ostr << seq->type << " " << seq->length << " ";
	    for(int index = 0; index < seq->length; ++index){
            ostr << inputdata::alphabet[seq->symbols[index]];
            if(inputdata::num_attributes != 0){
                ostr << inputdata::alphabet[seq->symbols[index]] << ":";
                for(int val = 0; val < inputdata::num_attributes-1; ++val){
                    ostr << seq->values[index][val] << ",";
                }
                ostr << seq->values[index][inputdata::num_attributes-1] << " ";
            }
            if(!seq->data[index].empty()){
                ostr << "/" << seq->data;
            }
            ostr << " ";
        }
        ostr << "\n";
    }
    return ostr.str();
};

*/

