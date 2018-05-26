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

/**
 * @brief Base class for refinements. Specialized to either
 * a point (merge), split, or color (adding of a new state).
 *
 */
class refinement{
public:
    double score;
	apta_node* right;

	virtual void print() const;
	virtual void print_short() const;
	virtual void doref(state_merger* m);
	virtual void undo(state_merger* m);
};

/**
 * @brief A merge-refinement assigns a score to the merge of
 * a left and right state. 
 *
 */
class merge_refinement : public refinement {
	apta_node* left;
	
public:
	merge_refinement(double s, apta_node* l, apta_node* r);

	virtual inline void print() const;
	virtual inline void print_short() const;
	virtual inline void doref(state_merger* m);
	virtual inline void undo(state_merger* m);
};

 /**
 * @brief A extend-refinement makes a blue state red. The
 * score is the size (frequency) of the state in the APTA.
 *
 */
class extend_refinement : public refinement {
public:
	extend_refinement(apta_node* r);

	virtual inline void print() const;
	virtual inline void print_short() const;
	virtual inline void doref(state_merger* m);
	virtual inline void undo(state_merger* m);
};

class split_refinement : public refinement {
    tail* split_point;
    int attribute;
	
public:
	split_refinement(double s, apta_node* l, tail* t, int a);

	virtual inline void print() const;
	virtual inline void print_short() const;
	virtual inline void doref(state_merger* m);
	virtual inline void undo(state_merger* m);
};


 /**
 * @brief Compare function for refinements, based on scores.
 *
 */
struct score_compare {
    inline bool operator()(refinement* r1, refinement* r2) const {
        if(r1->score == r2->score) return r1->right->size > r2->right->size;
        return r1->score > r2->score;
    }
};

#endif /* _REFINEMENT_H_ */
