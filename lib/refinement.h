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
#include <set>

class refinement;
class merge_refinement;
class extend_refinement;
struct score_compare;

typedef list<refinement*> refinement_list;
typedef set<refinement*, score_compare > refinement_set;

#include "apta.h"

class refinement{
public:
    double score;

	virtual void print() const;
	virtual void print_short() const;
	virtual void doref(state_merger* m);
	virtual void undo(state_merger* m);
};

class merge_refinement : public refinement {
	apta_node* left;
	apta_node* right;
	
public:
	merge_refinement(double s, apta_node* l, apta_node* r);

	virtual inline void print() const;
	virtual inline void print_short() const;
	virtual inline void doref(state_merger* m);
	virtual inline void undo(state_merger* m);
};

class extend_refinement : public refinement {
	apta_node* right;
	
public:
	extend_refinement(apta_node* r);

	virtual inline void print() const;
	virtual inline void print_short() const;
	virtual inline void doref(state_merger* m);
	virtual inline void undo(state_merger* m);
};

struct score_compare {
    inline bool operator()(refinement* left, refinement* right) const {
        return left->score < right->score;
    }
};

#endif /* _REFINEMENT_H_ */
