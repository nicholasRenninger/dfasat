#include "verify_dfa.h"

dfa::dfa(ifstream &input_stream){
	int num_states = -1;
	int alphabet_size = -1;
    
	input_stream >> num_states >> alphabet_size;
    input_stream >> start_state;

	states = vector<bool>(num_states);
	transitions = vector< vector<int> >(num_states);
	pos_counts = vector< vector<int> >(num_states);
	neg_counts = vector< vector<int> >(num_states);

    for(int i = 0; i < num_states; ++i){
		states[i] = false;
		transitions[i] = vector<int>(alphabet_size);
		pos_counts[i] = vector<int>(alphabet_size);
		neg_counts[i] = vector<int>(alphabet_size);
	}

 	for(int i = 0; i < num_states; ++i){
		for(int j = 0; j < alphabet_size; ++j){
			pos_counts[i][j] = 0;
			neg_counts[i][j] = 0;
        }
	}

	while(!input_stream.eof()){
		char id;
		input_stream >> id;
        
		if(id == 'a'){
		  int num, value;
		  input_stream >> num >> value;
          if(value == 1)
            states[num] = true;
          else
            states[num] = false;
		} else if(id == 't'){
		  int num1, num2, label;
		  input_stream >> num1 >> label >> num2;
		  transitions[num1][label] = num2;
		}
	}
}

bool dfa::check(ifstream &input){
	bool result = true;
	int num_lines;
	int alphabet_size;

	input >> num_lines >> alphabet_size;

	for(int i = 0; i < num_lines; ++i){
		int label, length;
		input >> label >> length;
		int state = start_state;
		for(int j = 0; j < length; ++j){
			int symbol;
			input >> symbol;

			if( label == 1 )
			    pos_counts[state][symbol] = pos_counts[state][symbol] + 1;
			else
			    neg_counts[state][symbol] = neg_counts[state][symbol] + 1;

			state = transitions[state][symbol];
		}
		if(states[state] != (label == 1)){
			cerr << "bad string " << i << " end in " << state << endl;
            result = false;
		} else {
			//cerr << "good string " << i << " end in " << state << endl;
        }
	}
 	for(int i = 0; i < pos_counts.size(); ++i){
		for(int j = 0; j < alphabet_size; ++j){
			cerr << i << " " << j << " " << pos_counts[i][j] + neg_counts[i][j] << endl;
        }
	}
    return result;
}

void dfa::classify(ifstream &input){
	int num_lines;
	int alphabet_size;
	int one = 0, zero = 0;

	input >> num_lines >> alphabet_size;

	for(int i = 0; i < num_lines; ++i){
		int label, length;
		input >> label >> length;
		int state = start_state;
		bool occurred = true;
		for(int j = 0; j < length; ++j){
			int symbol;
			input >> symbol;
			if(pos_counts[state][symbol] == 0) { occurred = false; }
			state = transitions[state][symbol];
		}
		if(occurred == false ){ cout << '0' << endl; zero++; }
		else if(occurred && states[state]) { cout << '1' << endl; one++; }
		else { cout << '0' << endl; zero++; }
	}
	cout << endl;

	cerr << "classify occurences 1: " << one << " 0: " << zero << endl;
}

int main(int argc, const char *argv[]){
	ifstream input(argv[1]);
	if(input.good()){
		dfa a(input);

		cerr << "created dfa" << endl;

		ifstream test(argv[2]);
		if(test.good()){
			if(a.check(test)){

				cerr << "tested without errors" << endl;

			ifstream cla(argv[3]);
			if(cla.good()){ a.classify(cla);

				cerr << "classified" << endl;
                } else {
                    cerr << "cannot classify" << endl;
                }
			
			} else {
				cerr << "tested with errors" << endl;
			}
		}
	} else {
        cerr << "cannot open input file" << endl;
    }
}
