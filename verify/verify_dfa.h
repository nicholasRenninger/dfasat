
#ifndef _TEST_DFA_H_
#define _TEST_DFA_H_

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

class dfa{
public:
	int start_state;
	vector<bool> states;
	vector< vector<int> > transitions;
	vector< vector<int> > pos_counts;
	vector< vector<int> > neg_counts;
    
	dfa(ifstream &input_stream);

	bool check(ifstream &input_stream);
	void classify(ifstream &input_stream);
};

#endif /* _TEST_DFA_H_ */
