/*
 *  RTI (real-time inference)
 *  Searcher.cpp, the header file for the search routines
 *  Currently, only a simple greedy (best-first) routine is implemented, search routines will be added later.
 *
 *  A refinement is either a point (merge), split, or color (adding of a new state) in the current real-time automaton, as in:
 *  Sicco Verwer and Mathijs de Weerdt and Cees Witteveen (2007),
 *  An algorithm for learning real-time automata,
 *  In Maarten van Someren and Sophia Katrenko and Pieter Adriaans (Eds.),
 *  Proceedings of the Sixteenth Annual Machine Learning Conference of Belgium and the Netherlands (Benelearn),
 *  pp. 128-135.
 *  
 *  Copyright 2009 - Sicco Verwer, jan-2009
 *  This program is released under the GNU General Public License
 *  Info online: http://www.gnu.org/licenses/quick-guide-gplv3.html
 *  Or in the file: licence.txt
 *  For information/questions contact: siccoverwer@gmail.com
 *
 *  I will try to keep this software updated and will also try to add new statistics or search methods.
 *  Also I will add comments to the source in the near future.
 *
 *  Feel free to adapt the code to your needs, please inform me of (potential) improvements.
 */

#ifndef _SEARCHER_H_
#define _SEARCHER_H_

using namespace std;

#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <map>

#include "timed_automaton.h"

extern int NODES;

class refinement;

typedef list<refinement> refinement_list;
typedef multimap<double, refinement, greater<double> > refinement_set;

timed_automaton* TA;

class refinement{
	apta_node* left;
	apta_node* right;
	
public:
	int ref_count;
	
	refinement(apta_node* l, apta_node* r);

	inline void print() const{
        cerr << "merge( " << l->number << " " << r->number << " )" << endl;
	};
	
	void doref(state_merger* m){
		//cerr << "do : "; print();
        m->merge(left, right)
	};
	
	void undo(state_merger* m){
		//cerr << "undo : "; print();
        m->undo_merge(left, right)
	};
};

#endif /* _SEARCHER_H_ */
