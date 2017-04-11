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

#ifndef _REFINEMENT_H_
#define _REFINEMENT_H_

using namespace std;

#include <list>
#include <queue>
#include <map>

class refinement;

typedef list<refinement> refinement_list;
typedef multimap<double, refinement, greater<double> > refinement_set;

class refinement{
	apta_node* left;
	apta_node* right;
	
public:
	refinement(apta_node* l, apta_node* r);

	inline void print() const{
        if(left == 0) cerr << "extend( " << right->number << " )" << endl;
        else cerr << "merge( " << left->number << " " << right->number << " )" << endl;
	};
	
	inline void doref(state_merger* m){
		//cerr << "do : "; print();
        if(left == 0) m->extend(right);
        else m->perform_merge(left, right);
	};
	
	inline void undo(state_merger* m){
		//cerr << "undo : "; print();
        if(left == 0) m->undo_extend(right);
        else m->undo_perform_merge(left, right);
	};
};

#endif /* _REFINEMENT_H_ */
